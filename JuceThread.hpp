//
//  JuceThread.hpp
//  Blocks
//
//  Created by Urban Lienert on 23.02.20.
//  Copyright © 2020 Urban Lienert. All rights reserved.
//

#include <BlocksHeader.h>
#include "BlockFinder.hpp"
#include "m_pd.h"

class JuceThread : private juce::Thread
{
public:
    JuceThread (const juce::String &threadName, t_outlet *a, t_outlet *b, t_outlet *c, t_outlet *d, size_t threadStackSize=0);
    ~JuceThread();
    
    void startThread();
    bool stopThread (int timeOutMilliseconds);
    void run() override;
    
    t_outlet *out_A;
    t_outlet *out_B;
    t_outlet *out_C;
    t_outlet *out_D;

    std::unique_ptr<BlockFinder> mBlockFinder;
    
private:

    
};
