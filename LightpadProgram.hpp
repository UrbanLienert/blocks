//
//  LightpadProgramm.hpp
//  Blocks
//
//  Created by Urban Lienert on 09.02.20.
//  Copyright © 2020 Urban Lienert. All rights reserved.
//

#include <BlocksHeader.h>

typedef enum {
    mLogo,
    mDrumpads,
    mFaders,
    mXYZpad,
    mPaint,
    mMixer
} b_mode;

struct LightpadProgram   : public juce::Block::Program
{
    LightpadProgram (juce::Block&);

private:
    juce::String getLittleFootProgram() override;
};

