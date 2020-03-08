//
//  BlockFinder.hpp
//  Blocks
//
//  Created by Urban Lienert on 02.02.20.
//  Copyright © 2020 Urban Lienert. All rights reserved.
//

#pragma once

#ifndef JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED
 #define JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED 1
#endif

#ifndef JUCE_DONT_DECLARE_PROJECTINFO
 #define JUCE_DONT_DECLARE_PROJECTINFO 1
#endif

#include <BlocksHeader.h>
#include "BlockComponent.hpp"
#include "m_pd.h"

// Monitors a PhysicalTopologySource for changes to the connected BLOCKS and
// prints some information about the BLOCKS that are available.
class BlockFinder : private juce::TopologySource::Listener
{
public:
    // Register as a listener to the PhysicalTopologySource, so that we receive
    // callbacks in topologyChanged().
    BlockFinder();
    ~BlockFinder();
    
    t_outlet *out_A;
    t_outlet *out_B;
    t_outlet *out_C;
    t_outlet *out_D;

    void setPdNameForSerial(const char *serial, const char *name);
    void doBlockCommand(t_symbol *name, int argc, t_atom *argv);
    void pollInfos();
    
        
private:
    // Called by the PhysicalTopologySource when the BLOCKS topology changes.
    void topologyChanged() override;

    // The PhysicalTopologySource member variable which reports BLOCKS changes.
    juce::PhysicalTopologySource pts;
        
    //std::unique_ptr<BlockComponent> connectedBlockComponent;
    juce::StringPairArray* namesAndUIDs = nullptr;
    
    // new for multiple Blocks
    juce::OwnedArray<BlockComponent> blockComponents;
    //BlockComponent* masterBlockComponent = nullptr;

    void updateComponents();
    void outputTopology();
    
    JUCE_LEAK_DETECTOR (BlockFinder)
    
};
