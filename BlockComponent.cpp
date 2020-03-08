//
//  BlockComponent.cpp
//  Blocks
//
//  Created by Urban Lienert on 05.02.20.
//  Copyright © 2020 Urban Lienert. All rights reserved.
//

#include "BlockComponent.hpp"

using namespace juce;

BlockComponent::BlockComponent(Block::Ptr blockToUse) {
    
    block = blockToUse;
    pdName = new String(block->getDeviceDescription().upToFirstOccurrenceOf(String(" "), false, false).toLowerCase());

    blockMode = mLogo;
    gridSize = 0;
    
    messageSet = new NamedValueSet();
    rwLock = new ReadWriteLock();
    startTimer(50);
    
    
    // Register BlockComponent as a listener to the touch surface
    if (auto touchSurface = block->getTouchSurface()) {
        touchSurface->addListener (this);
        block->addProgramEventListener(this);
    }

    // Register BlockComponent as a listener to any buttons
    for (auto button : block->getButtons())
        button->addListener (this);
    
    // If it's a Lightpad load the LightpadProgram
    if (block->getType() == Block::lightPadBlock) {
        block->setProgram (new LightpadProgram (*block));
    }
    
}

BlockComponent::~BlockComponent() {
    rwLock->~ReadWriteLock();
    rwLock = nullptr;
    // Remove any listeners
    if (auto touchSurface = block->getTouchSurface()) {
        block->removeProgramEventListener(this);
        touchSurface->removeListener (this);
    }
    for (auto button : block->getButtons())
        button->removeListener (this);
}

void BlockComponent::setLightpadMode(String name) {
    if (name.compare("logo")==0) {
        blockMode = mLogo;
    } else if (name.compare("pads")==0) {
        blockMode = mDrumpads;
    } else if (name.compare("faders")==0) {
        blockMode = mFaders;
    } else if (name.compare("xyz")==0) {
        blockMode = mXYZpad;
    } else if (name.compare("paint")==0) {
        blockMode = mPaint;
    } else if (name.compare("mixer")==0) {
        blockMode = mMixer;
    }
    sendStampedMessage(0, 0, 0, 0, (uint32)blockMode);
}

void BlockComponent::setGridSize(int size) {
    gridSize = size;
    sendStampedMessage(0, 1, 0, 0, (uint32)gridSize);
}

void BlockComponent::setColors(juce::OwnedArray<juce::LEDColour>* colors) {
    if (auto program = dynamic_cast<LightpadProgram*> (block->getProgram())) {
        int c = 0;

        for (int i = 0; i<8; i++) {
            uint32 color1 = colors->operator[](c)->getARGB();
            c++;
            if (c>=colors->size()) c = 0;
            uint32 color2 = colors->operator[](c)->getARGB();
            c++;
            if (c>=colors->size()) c = 0;
            uint32 color3 = colors->operator[](c)->getARGB();
            c++;
            if (c>=colors->size()) c = 0;

            uint8 param1 = (color1 & 0x00ff0000) >> 16;
            uint32 param2 = ((color1 & 0x0000ffff) << 16) + ((color2 & 0x00ffff00) >> 8);
            uint32 param3 = ((color2 & 0x000000ff) << 24) + (color3 & 0x00ffffff);
            
            sendStampedMessage(1, i*3, param1, param2, param3);
        }
        uint32 color25 = colors->operator[](c)->getARGB();
        uint32 param3 = color25 + 0xff000000;
        sendStampedMessage(2, 24, 0, 0, param3);
    }
}

void BlockComponent::setLEDColor(int x, int y, LEDColour *colour) {
    int c = colour->getARGB();
    int ledNr = x + y * 15;
    sendStampedMessage(4, ledNr, 0, 0, c);
}

void BlockComponent::setRectColor(int x, int y, int w, int h, LEDColour *colour) {
    int c = colour->getARGB();
    int ledNr = x + y * 15;
    sendStampedMessage(6, ledNr, w, h, c);
}

void BlockComponent::setCircleColor(int x, int y, int r, LEDColour *colour) {
    int c = colour->getARGB();
    sendStampedMessage(7, x, y, r, c);
}

void BlockComponent::setTriangleColor(int x, int y, int s, int deg, juce::LEDColour *colour) {
    int c = colour->getARGB();
    int param2 = (s << 16) + (deg & 0xffff);
    sendStampedMessage(8, x, y, param2, c);
}

void BlockComponent::clearScreen() {
    sendStampedMessage(5, 0, 0, 0, 0);
}

void BlockComponent::setFaderValue(int index, float value) {
    sendStampedMessage(3, index-1, 0, 0, (uint32)(value*1e6));
}

void BlockComponent::setMixerButtonValue(int index, float value) {
    sendStampedMessage(3, index-1, 0, 1, (uint32)(value*1e6));
}

void BlockComponent::setMixerFaderValue(int index, float value) {
    sendStampedMessage(3, index-1, 0, 2, (uint32)(value*1e6));
}

void BlockComponent::setSettingsValue(juce::String name, int value) {
    int maxIndex = block->getMaxConfigIndex();
    for (int i=0; i<maxIndex; i++) {
        Block::ConfigMetaData metaData = block->getLocalConfigMetaData(i);
        if (metaData.name.removeCharacters(". ").toLowerCase().compare(name.toLowerCase())==0) {
            block->setLocalConfigValue(metaData.item, value);
            post((metaData.name + ": %i").toUTF8(), value);
            break;
        }
    }
}

void BlockComponent::setSettingsValue(juce::String name, juce::String option) {
    int maxIndex = block->getMaxConfigIndex();
    for (int i=0; i<maxIndex; i++) {
        Block::ConfigMetaData metaData = block->getLocalConfigMetaData(i);
        if (metaData.name.removeCharacters(". ").toLowerCase().compare(name.toLowerCase())==0) {
            for (int j = 0; j < metaData.numOptionNames; j++) {
                if (metaData.optionNames[j].removeCharacters("- ").toLowerCase().compare(option.toLowerCase())==0) {
                    block->setLocalConfigValue(metaData.item, j);
                    post((metaData.name + ": " + metaData.optionNames[j]).toUTF8());
                    break;
                }
            }
        }
    }
}

void BlockComponent::outputInfos() {
    t_symbol *name = gensym(pdName->toStdString().c_str());

    float batteryLevel = block->getBatteryLevel();
    t_atom at[2];
    SETSYMBOL(at, gensym("battery"));
    SETFLOAT(at + 1, (t_float)static_cast<float>(batteryLevel));
    outlet_anything(out_info, name, 2, at);
    float rotation = block->getRotation();
    SETSYMBOL(at, gensym("rotation"));
    SETFLOAT(at + 1, (t_float)static_cast<float>(rotation));
    outlet_anything(out_info, name, 2, at);
    SETSYMBOL(at, gensym("master"));
    SETFLOAT(at + 1, (t_float)static_cast<float>(block->isMasterBlock()));
    outlet_anything(out_info, name, 2, at);
    SETSYMBOL(at, gensym("charging"));
    SETFLOAT(at + 1, (t_float)static_cast<float>(block->isBatteryCharging()));
    outlet_anything(out_info, name, 2, at);
}

void BlockComponent::sendStampedMessage(juce::uint32 commandNr, juce::uint32 subCommandNr, juce::uint8 param1, juce::uint32 param2, juce::uint32 param3) {
    const MessageManagerLock *mmLock = new MessageManagerLock();
    
    // new: 6bit command nr, 8bit subcommand nr, 10bit timestamp, 8bit data byte ( receive 23bit data )
    
    juce::Block::ProgramEventMessage *message = new juce::Block::ProgramEventMessage();
    uint32 millis = Time().getMillisecondCounter();
    millis = (uint32)(millis & 0x3FF) << 8;
    commandNr = commandNr << 26;
    subCommandNr = subCommandNr << 18;
    message->values[0] = millis + commandNr + subCommandNr + param1;
    message->values[1] = param2;
    message->values[2] = param3;
    
    addMessageToCheck(message);
    block->sendProgramEvent(*message);
    
    mmLock->~MessageManagerLock();
    mmLock = nullptr;
}


void BlockComponent::addMessageToCheck(juce::Block::ProgramEventMessage *message) {
    rwLock->enterWrite();
    uint32 stamp = (message->values[0] >> 18 ) & 0x3FFF; // command and subcommand
    MemoryBlock *memoryBlock = new MemoryBlock(message->values, sizeof(int32)*3);
    var *value = new var(*memoryBlock);
    String *name = new String(stamp);
    Identifier *identifier = new Identifier(*name);
    messageSet->set(*identifier, *value);
    memoryBlock->~MemoryBlock();
    value->~var();
    rwLock->exitWrite();
}

void BlockComponent::timerCallback() {
    rwLock->enterWrite();
    for (auto &obj : *messageSet) {
        var value = obj.value;
        MemoryBlock *memoryBlock = value.getBinaryData();
        void *values = memoryBlock->getData();
        uint32 param1 = ((uint32 *)values)[0];
        uint32 millis = (param1 >> 8) & 0x3FF;
        uint32 now = Time().getMillisecondCounter();
        now = (uint32)(now & 0x3FF);
        int diff = now - millis;
        if (diff<0) diff += 1023;
        if (diff>100) {
            uint32 command = (param1 >> 26) & 0x3F;
            uint32 subCommand = (param1 >> 18) & 0xFF;
            uint8 param1 = ((uint32 *)values)[0] & 0x000000ff;
            uint32 param2 = ((uint32 *)values)[1];
            uint32 param3 = ((uint32 *)values)[2];
            //post("should resend packet %u - %u / %u - %u / %i", command, subCommand, param2, param3, diff);
            sendStampedMessage(command, subCommand, param1, param2, param3);
        }
    }
    rwLock->exitWrite();
}

void BlockComponent::checkMessages(int param1) {
    rwLock->enterWrite();
    uint32 stamp = (param1 >> 18 ) & 0x3FFF; // command and subcommand
    uint32 millis = (param1 >> 8) & 0x3FF;
    
    String *name = new String(stamp);
    Identifier *identifier = new Identifier(*name);
    uint32 command = (param1 >> 26 ) & 0x3F; // command

    if (messageSet->contains(*identifier)) {
        uint32 now = Time().getMillisecondCounter();
        now = (uint32)(now & 0x3FF);
        int diff = now - millis;
        if (diff<0) diff += 1023;
        //printf("got message %i returned: (time: %i)\n", messageSet->indexOf(*identifier), diff);
        if (command>=4 && command<=8) {
            if (messageSet->indexOf(*identifier)==0) {
                messageSet->remove(*identifier);
            }
        } else {
            messageSet->remove(*identifier);
        }
    } else {
        //printf("didn't found the message\n");
    }
    rwLock->exitWrite();
}

int BlockComponent::padIndexForTouch(const TouchSurface::Touch& t) {
    int row = int (t.startY * (0.95 / 2.0) * float (gridSize)) + 1;
    int col = int (t.startX * (0.95 / 2.0) * float (gridSize)) + 1;
    return (gridSize * (gridSize - row)) + col;
}

// juce::TouchSurface::Listener

void BlockComponent::touchChanged (TouchSurface&, const TouchSurface::Touch& t) {
    if (t.isTouchStart) {
        if (blockMode==mDrumpads) {
            int padIndex = padIndexForTouch(t);
            lastTouched = padIndex;
            t_atom at[3];
            SETSYMBOL(at, gensym("pad"));
            SETFLOAT(at + 1, (t_float)static_cast<float>(padIndex));
            SETFLOAT(at + 2, (t_float)t.zVelocity);
            t_symbol *name = gensym(pdName->toStdString().c_str());
            outlet_anything(out_action, name, 3, at);
        }
    } else if (t.isTouchEnd) {
        if (blockMode==mDrumpads) {
            int padIndex = padIndexForTouch(t);
            t_atom at[3];
            SETSYMBOL(at, gensym("pad"));
            SETFLOAT(at + 1, (t_float)static_cast<float>(padIndex));
            SETFLOAT(at + 2, (t_float)0);
            t_symbol *name = gensym(pdName->toStdString().c_str());
            outlet_anything(out_action, name, 3, at);
        }
    } else {
        if (blockMode==mDrumpads) {
            int padIndex = padIndexForTouch(t);
            if (padIndex==lastTouched) {
                float x = t.x - t.startX;
                float y = t.y - t.startY;
                float z = t.z;
                t_atom at[4];
                SETSYMBOL(at, gensym("bend"));
                SETFLOAT(at + 1, x);
                SETFLOAT(at + 2, y);
                SETFLOAT(at + 3, z);
                t_symbol *name = gensym(pdName->toStdString().c_str());
                outlet_anything(out_action, name, 4, at);
            }
        }
    }
    if (blockMode==mXYZpad || blockMode==mPaint) {
        float phase = 2;
        if (t.isTouchStart) phase = 1;
        else if (t.isTouchEnd) phase = 0;
        t_atom at[6];
        if (blockMode==mXYZpad) SETSYMBOL(at, gensym("touch"));
        else SETSYMBOL(at, gensym("draw"));
        SETFLOAT(at + 1, (t_float)static_cast<float>(t.index));
        SETFLOAT(at + 2, (t_float)phase);
        SETFLOAT(at + 3, (t_float)t.x);
        SETFLOAT(at + 4, (t_float)t.y);
        SETFLOAT(at + 5, (t_float)t.z);
        t_symbol *name = gensym(pdName->toStdString().c_str());
        outlet_anything(out_action, name, 6, at);
    }
}

// juce::ControlButton::Listener

void BlockComponent::buttonPressed  (ControlButton& b, Block::Timestamp t) {
    t_atom at[2];
    SETSYMBOL(at, gensym("button"));
    SETFLOAT(at + 1, (t_float)1);
    t_symbol *name = gensym(pdName->toStdString().c_str());
    outlet_anything(out_action, name, 2, at);
}

void BlockComponent::buttonReleased (ControlButton& b, Block::Timestamp t) {
    t_atom at[2];
    SETSYMBOL(at, gensym("button"));
    SETFLOAT(at + 1, (t_float)0);
    t_symbol *name = gensym(pdName->toStdString().c_str());
    outlet_anything(out_action, name, 2, at);
}

// juce::Block::ProgramEventListener

void BlockComponent::handleProgramEvent (juce::Block &source, const juce::Block::ProgramEventMessage &message) {
    
    uint32 command = (message.values[0] >> 26 ) & 0x3F; // command
    if (command<10) {
        // return packets
        checkMessages(message.values[0]);
    } else {
        // commands from blocks
        switch (command) {
            case 10: { // faders
                int index = (message.values[0] & 0x000000ff) + 1;
                int phase = message.values[1];
                float value = (float)message.values[2] / 1e6;
                
                t_atom at[4];
                SETSYMBOL(at, gensym("fader"));
                SETFLOAT(at + 1, (t_float)static_cast<float>(index));
                SETFLOAT(at + 2, (t_float)static_cast<float>(phase));
                SETFLOAT(at + 3, (t_float)value);
                t_symbol *name = gensym(pdName->toStdString().c_str());
                outlet_anything(out_action, name, 4, at);
                break;
            }
            case 11: { // mixer
                bool isFader = (bool)(message.values[0] & 0x000000ff);
                int index = message.values[1] + 1;
                float value = (float)message.values[2] / 1e6;
                if (isFader) {
                    t_atom at[4];
                    SETSYMBOL(at, gensym("mixer"));
                    SETSYMBOL(at + 1, gensym("fader"));
                    SETFLOAT(at + 2, (t_float)static_cast<float>(index));
                    SETFLOAT(at + 3, (t_float)value);
                    t_symbol *name = gensym(pdName->toStdString().c_str());
                    outlet_anything(out_action, name, 4, at);
                } else {
                    t_atom at[4];
                    SETSYMBOL(at, gensym("mixer"));
                    SETSYMBOL(at + 1, gensym("button"));
                    SETFLOAT(at + 2, (t_float)static_cast<float>(index));
                    bool on = value!=0;
                    SETFLOAT(at + 3, (t_float)on);
                    t_symbol *name = gensym(pdName->toStdString().c_str());
                    outlet_anything(out_action, name, 4, at);
                }
                break;
            }
            default:
                break;
        }
    }
};
