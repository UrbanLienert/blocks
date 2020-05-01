//
//  BlockComponent.hpp
//  Blocks
//
//  Created by Urban Lienert on 05.02.20.
//  Copyright © 2020 Urban Lienert. All rights reserved.
//

#include <BlocksHeader.h>
#include "m_pd.h"
#include "LightpadProgram.hpp"

class BlockComponent : private juce::TouchSurface::Listener,
                       private juce::ControlButton::Listener,
                       private juce::Block::ProgramEventListener,
                       private juce::Timer
{
public:
    BlockComponent (juce::Block::Ptr blockToUse, bool loadProgram);
    ~BlockComponent();
    
    juce::Block::Ptr block;
    juce::String *pdName;
    
    b_mode blockMode;
    int gridSize;
    int lastTouched;
    
    t_outlet *out_action;
    t_outlet *out_info;
    
    juce::NamedValueSet *messageSet;
    
    // set Programm as default
    void setDefault();
    
    // set Mode in Lightpad Program
    void setLightpadMode(juce::String name);
    
    // set Grid Size
    void setGridSize(int size);
    
    // set Fader values
    void setFaderValue(int index, float value);
    
    // set Mixer values
    void setMixerFaderValue(int index, float value);
    void setMixerButtonValue(int index, float value);
    
    // set Object Colours
    void setColors(juce::OwnedArray<juce::LEDColour>* colors);
    
    // set LED Color
    void setLEDColor(int x, int y, juce::LEDColour *colour);
    void setRectColor(int x, int y, int w, int h, juce::LEDColour *colour);
    void setCircleColor(int x, int y, int r, juce::LEDColour *colour);
    void setTriangleColor(int x, int y, int s, int deg, juce::LEDColour *colour);
    void setNumberColor(int n, juce::LEDColour *colour);
    void hideNumberColor();
    void clearScreen();
    
    // set Local Settings
    void setSettingsValue(juce::String name, int value);
    void setSettingsValue(juce::String name, juce::String option);

    // output Infos
    void outputInfos();
    
    // messages
    void addMessageToCheck(juce::Block::ProgramEventMessage *message);
    void checkMessages(int param1);
    void timerCallback() override;
    void sendStampedMessage(juce::uint32 commandNr, juce::uint32 subCommandNr, juce::uint8 param1, juce::uint32 param2, juce::uint32 param3);
    
    juce::ReadWriteLock *rwLock;
    
private:
    int padIndexForTouch(const juce::TouchSurface::Touch& t);
    
    /** Overridden from TouchSurface::Listener */
    void touchChanged (juce::TouchSurface&, const juce::TouchSurface::Touch& t) override;
    /** Overridden from ControlButton::Listener */
    void buttonPressed  (juce::ControlButton& b, juce::Block::Timestamp t) override;
    /** Overridden from ControlButton::Listener */
    void buttonReleased (juce::ControlButton& b, juce::Block::Timestamp t) override;
    
    /** Overridden from Block::ProgramEventListener*/
    void handleProgramEvent (juce::Block &source, const juce::Block::ProgramEventMessage &message) override;
    
    JUCE_LEAK_DETECTOR (BlockComponent)
};
