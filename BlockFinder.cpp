//
//  BlockFinder.cpp
//  Blocks
//
//  Created by Urban Lienert on 02.02.20.
//  Copyright © 2020 Urban Lienert. All rights reserved.
//

#include "BlockFinder.hpp"
#include "m_pd.h"

using namespace juce;

typedef struct cf_data
{
    BlockComponent *component;
    String name;
    int value;
    String option;
} cf_data;

// callback function for message thread
void* settingValueFunction (void* d) {
    cf_data data = *(cf_data *)d;
    BlockComponent* component = data.component;
    component->setSettingsValue(data.name, data.value);
    return nullptr;
};

void* settingsOptionFunction (void* d) {
    cf_data data = *(cf_data *)d;
    BlockComponent* component = data.component;
    component->setSettingsValue(data.name, data.option);
    return nullptr;
};

BlockFinder::BlockFinder()
{
    // Register to receive topologyChanged() callbacks from pts.
    pts.addListener (this);
    namesAndUIDs = new StringPairArray;
}

BlockFinder::~BlockFinder() {
    pts.setActive(false);
    namesAndUIDs->~StringPairArray();
}

void BlockFinder::topologyChanged()
{
    auto currentTopology = pts.getCurrentTopology();
    
    for (auto& component : blockComponents) {
        bool found = false;
        for (auto& block : currentTopology.blocks) {
            if (component->block->uid==block->uid) {
                found = true;
                break;
            }
        }
        if (!found) {
            blockComponents.removeObject(component);
        }
    }

    post(("Detected " + String (currentTopology.blocks.size()) + " blocks:").toUTF8());
    post("");

    for (auto& block : currentTopology.blocks)
    {
        post(("    Description:   " + block->getDeviceDescription()).toUTF8());
        post(("    Serial number: " + block->serialNumber).toUTF8());
        post("");
        
        bool found = false;
        for (auto& component : blockComponents) {
            if (component->block->uid==block->uid) {
                found = true;
                break;
            }
        }
        if (!found) {
            BlockComponent *component = new BlockComponent(block);
            component->out_action = out_A;
            component->out_info = out_B;
            blockComponents.add(component);
            String serial = namesAndUIDs->getValue(*component->pdName, String());
            if (serial.length()==0) {
                setPdNameForSerial(component->block->serialNumber.toUTF8(), component->pdName->toUTF8());
            }
        }
    }
        
    // setting pdNames in components
    updateComponents();
    
    if (pts.isActive()) {
        // send bang to output
        outlet_bang(out_D);
        outputTopology();
    }
}

void BlockFinder::setPdNameForSerial(const char *serial, const char *name) {
    namesAndUIDs->set(String(name), String(serial));
    updateComponents();
}

void BlockFinder::doBlockCommand(t_symbol *name, int argc, t_atom *argv) {
    String serial = namesAndUIDs->getValue(String(name->s_name), String());
    if (serial.length()>0) {
        // found the block
        for (BlockComponent* component : blockComponents) {
            if (component->block->serialNumber.compare(serial)==0) {
                if (argc>0) {
                    t_atom cAtom = argv[0];
                    if (cAtom.a_type==A_SYMBOL) {
                        String command = String(cAtom.a_w.w_symbol->s_name);
                        // mode command
                        if (command.compare("mode")==0 && argc>1) {
                            t_atom pAtom = argv[1];
                            if (pAtom.a_type==A_SYMBOL) {
                                component->setLightpadMode(String(pAtom.a_w.w_symbol->s_name));
                            }
                            if (argc>2) {
                                t_atom gAtom = argv[2];
                                if (pAtom.a_type==A_SYMBOL && gAtom.a_type==A_FLOAT) {
                                    int size = (int)gAtom.a_w.w_float;
                                    if (size<1) size = 1;
                                    if (size>5) size = 5;
                                    component->setGridSize(size);
                                }
                            } else {
                                component->setGridSize(2);
                            }
                        }
                        // color command
                        else if (command.compare("color")==0 && argc>1) {
                            // set color for pads, faders, etc.
                            OwnedArray<LEDColour> *colors = new OwnedArray<LEDColour>();
                            int numColors = argc - 1;
                            for (int i=1; i<=numColors; i++) {
                                t_atom hAtom = argv[i];
                                if (hAtom.a_type==A_SYMBOL) {
                                    String *hexString = new String(hAtom.a_w.w_symbol->s_name);
                                    uint32 argb = 0xff000000 + hexString->getHexValue32();
                                    LEDColour *color = new LEDColour(argb);
                                    colors->add(color);
                                }
                            }
                            if (colors->size()>0) {
                                component->setColors(colors);
                            }
                            colors->~OwnedArray();
                        }
                        // set fader value command
                        else if (command.compare("fader")==0 && argc>2) {
                            t_atom fAtom1 = argv[1];
                            t_atom fAtom2 = argv[2];
                            if (fAtom1.a_type==A_FLOAT && fAtom2.a_type==A_FLOAT) {
                                int index = (int)fAtom1.a_w.w_float;
                                float value = fAtom2.a_w.w_float;
                                component->setFaderValue(index, value);
                            }
                        }
                        // set mixer fader and button value command
                        else if (command.compare("mixer")==0 && argc>3) {
                            t_atom sAtom = argv[1];
                            t_atom fAtom1 = argv[2];
                            t_atom fAtom2 = argv[3];
                            if (sAtom.a_type==A_SYMBOL && fAtom1.a_type==A_FLOAT && fAtom2.a_type==A_FLOAT) {
                                String subCommand = String(sAtom.a_w.w_symbol->s_name);
                                if (subCommand.compare("button")==0) {
                                    int index = (int)fAtom1.a_w.w_float;
                                    float value = fAtom2.a_w.w_float;
                                    component->setMixerButtonValue(index, value);
                                } else if (subCommand.compare("fader")==0) {
                                    int index = (int)fAtom1.a_w.w_float;
                                    float value = fAtom2.a_w.w_float;
                                    component->setMixerFaderValue(index, value);
                                }

                            }
                        }
                        // set led color command
                        else if (command.compare("led")==0 && argc>3) {
                            t_atom fAtom1 = argv[1];
                            t_atom fAtom2 = argv[2];
                            t_atom sAtom = argv[3];
                            if (fAtom1.a_type==A_FLOAT && fAtom2.a_type==A_FLOAT && sAtom.a_type==A_SYMBOL) {
                                int x = (int)fAtom1.a_w.w_float;
                                int y = fAtom2.a_w.w_float;
                                String *hexString = new String(sAtom.a_w.w_symbol->s_name);
                                uint32 argb = 0xff000000 + hexString->getHexValue32();
                                LEDColour *color = new LEDColour(argb);
                                component->setLEDColor(x - 1, y - 1, color);
                            }
                        }
                        // draw rect with color command
                        else if (command.compare("rect")==0 && argc>5) {
                            t_atom fAtom1 = argv[1];
                            t_atom fAtom2 = argv[2];
                            t_atom fAtom3 = argv[3];
                            t_atom fAtom4 = argv[4];
                            t_atom sAtom = argv[5];
                            if (fAtom1.a_type==A_FLOAT && fAtom2.a_type==A_FLOAT && fAtom3.a_type==A_FLOAT && fAtom4.a_type==A_FLOAT && sAtom.a_type==A_SYMBOL) {
                                int x = (int)fAtom1.a_w.w_float;
                                int y = (int)fAtom2.a_w.w_float;
                                int w = (int)fAtom3.a_w.w_float;
                                int h = (int)fAtom4.a_w.w_float;
                                String *hexString = new String(sAtom.a_w.w_symbol->s_name);
                                uint32 argb = 0xff000000 + hexString->getHexValue32();
                                LEDColour *color = new LEDColour(argb);
                                component->setRectColor(x-1, y-1, w, h, color);
                            }
                        }
                        // draw circle with color command
                        else if (command.compare("circle")==0 && argc>4) {
                            t_atom fAtom1 = argv[1];
                            t_atom fAtom2 = argv[2];
                            t_atom fAtom3 = argv[3];
                            t_atom sAtom = argv[4];
                            if (fAtom1.a_type==A_FLOAT && fAtom2.a_type==A_FLOAT && fAtom3.a_type==A_FLOAT && sAtom.a_type==A_SYMBOL) {
                                int x = (int)fAtom1.a_w.w_float;
                                int y = (int)fAtom2.a_w.w_float;
                                int r = (int)fAtom3.a_w.w_float;
                                String *hexString = new String(sAtom.a_w.w_symbol->s_name);
                                uint32 argb = 0xff000000 + hexString->getHexValue32();
                                LEDColour *color = new LEDColour(argb);
                                component->setCircleColor(x-1, y-1, r, color);
                            }
                        }
                        // draw triangle with color command
                        else if (command.compare("triangle")==0 && argc>5) {
                            t_atom fAtom1 = argv[1];
                            t_atom fAtom2 = argv[2];
                            t_atom fAtom3 = argv[3];
                            t_atom fAtom4 = argv[4];
                            t_atom sAtom = argv[5];
                            if (fAtom1.a_type==A_FLOAT && fAtom2.a_type==A_FLOAT && fAtom3.a_type==A_FLOAT && fAtom4.a_type==A_FLOAT && sAtom.a_type==A_SYMBOL) {
                                int x = (int)fAtom1.a_w.w_float;
                                int y = (int)fAtom2.a_w.w_float;
                                int s = (int)fAtom3.a_w.w_float;
                                int d = (int)fAtom4.a_w.w_float;
                                String *hexString = new String(sAtom.a_w.w_symbol->s_name);
                                uint32 argb = 0xff000000 + hexString->getHexValue32();
                                LEDColour *color = new LEDColour(argb);
                                component->setTriangleColor(x-1, y-1, s, d, color);
                            }
                        }
                        // clear screen (drawing)
                        else if (command.compare("clear")==0) {
                            component->clearScreen();
                        }
                        // set block settings command
                        else if (command.compare("set")==0 && argc>2) {
                            t_atom sAtom = argv[1];
                            t_atom fAtom1 = argv[2];
                            String setting = String(sAtom.a_w.w_symbol->s_name);
                            if (sAtom.a_type==A_SYMBOL) {
                                MessageManager *messageManager = MessageManager::getInstance();
                                cf_data data;
                                data.component = component;
                                data.name = setting;
                                if (fAtom1.a_type==A_FLOAT) {
                                    // value
                                    int value = (int)fAtom1.a_w.w_float;
                                    data.value = value;
                                    // we have to call this in the messag thread
                                    messageManager->callFunctionOnMessageThread(settingValueFunction, &data);
                                } else if (fAtom1.a_type==A_SYMBOL) {
                                    // option
                                    String option = String(fAtom1.a_w.w_symbol->s_name);
                                    data.option = option;
                                    // we have to call this in the messag thread
                                    messageManager->callFunctionOnMessageThread(settingsOptionFunction, &data);
                                }
                            }
                        }
                        else {
                            error("no method for '%s'", command.toStdString().c_str());
                        }
                    }
                }
            }
        }
    } else {
        error("block '%s' not found", name->s_name);
    }
}

void BlockFinder::pollInfos() {
    for (BlockComponent* component : blockComponents) {
        component->outputInfos();
    }
}

void BlockFinder::updateComponents() {
    const MessageManagerLock *mmLock = new MessageManagerLock();

    for (BlockComponent* component : blockComponents) {
        StringArray names = namesAndUIDs->getAllKeys();
        for (String name : names) {
            String serial = namesAndUIDs->operator[](name);
            if (serial.compare(component->block->serialNumber)==0) {
                component->pdName = new String(name);
            }
        }
    }
    mmLock->~MessageManagerLock();
    mmLock = nullptr;
}

void BlockFinder::outputTopology() {
    auto currentTopology = pts.getCurrentTopology();

    for (BlockComponent* component : blockComponents) {
        t_symbol *name = gensym(component->pdName->toStdString().c_str());
        t_atom at[3];

        Block::Array connectedBlocks = currentTopology.getDirectlyConnectedBlocks(component->block->uid);
        for (auto& connectedBlock : connectedBlocks) {
            
            String blockName = String();
            for (BlockComponent* component : blockComponents) {
                if (component->block->uid==connectedBlock->uid) {
                    blockName = *component->pdName;
                }
            }
            
            Array<BlockDeviceConnection> connections = currentTopology.getConnectionsBetweenBlocks(component->block->uid, connectedBlock->uid);
            for (auto& connection : connections) {
                Block::ConnectionPort port;
                if (connection.device1==component->block->uid) {
                    port = connection.connectionPortOnDevice1;
                } else {
                    port = connection.connectionPortOnDevice2;
                }
                String edge = String();
                switch (port.edge) {
                    case juce::Block::ConnectionPort::DeviceEdge::north:
                        edge = String("north");
                        break;
                    case juce::Block::ConnectionPort::DeviceEdge::east:
                        edge = String("east");
                        break;
                    case juce::Block::ConnectionPort::DeviceEdge::south:
                        edge = String("south");
                        break;
                    case juce::Block::ConnectionPort::DeviceEdge::west:
                        edge = String("west");
                        break;
                    default:
                        break;
                }
                SETSYMBOL(at, gensym(edge.toStdString().c_str()));
                SETFLOAT(at + 1, (t_float)static_cast<float>(port.index));
                SETSYMBOL(at + 2, gensym(blockName.toStdString().c_str()));
                outlet_anything(out_C, name, 3, at);
            }
        }
    }
}

