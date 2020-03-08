//
//  blocks.cpp
//  Blocks
//
//  Created by Urban Lienert on 02.02.20.
//  Copyright © 2020 Urban Lienert. All rights reserved.
//

#include "m_pd.h"
#include "BlockFinder.hpp"
#include <BlocksHeader.h>
#include "JuceThread.hpp"

// Pure Data 'class' declaration
static t_class *blocks_class = NULL;

// workaround for juce message thread on macOS and pure data, because we are not on the main thread
#if JUCE_MAC
    CF_EXPORT CFRunLoopRef CFRunLoopGetMain(void) { return CFRunLoopGetCurrent(); };
#endif

// struct definition for blocks class
typedef struct {
    t_object x_obj;
    t_outlet *out_A;
    t_outlet *out_B;
    t_outlet *out_C;
    t_outlet *out_D;
    std::unique_ptr<JuceThread> juceThread;
} t_blocks;

// function declarations
static void *blocks_new();
void blocks_free(t_blocks *x);
extern "C" void blocks_setup(void);
static void blocks_setname(t_blocks *x, t_symbol *serial, t_symbol *name);
static void blocks_command(t_blocks *x, t_symbol *s, int argc, t_atom *argv);
static void blocks_bang(t_blocks *x);

static void *blocks_new()
{
    t_blocks *x = (t_blocks *)pd_new(blocks_class);
    x->out_A = outlet_new(&x->x_obj, &s_anything);
    x->out_B = outlet_new(&x->x_obj, &s_anything);
    x->out_C = outlet_new(&x->x_obj, &s_anything);
    x->out_D = outlet_new(&x->x_obj, &s_bang);

    x->juceThread = {std::make_unique<JuceThread>(juce::String("blockThread"), x->out_A, x->out_B, x->out_C, x->out_D)};
    x->juceThread->startThread();    
    return (x);
}

void blocks_free(t_blocks *x) {
    x->juceThread->stopThread(1000);
    outlet_free(x->out_A);
    outlet_free(x->out_B);
    outlet_free(x->out_C);
    outlet_free(x->out_D);
    x->juceThread = nullptr;
}

extern "C" void blocks_setup(void)
{
    blocks_class = class_new(gensym("blocks"), (t_newmethod)blocks_new,
        (t_method)blocks_free, sizeof(t_blocks), CLASS_DEFAULT, A_DEFFLOAT, A_DEFFLOAT, A_NULL);
    
    class_addmethod(blocks_class, (t_method)blocks_setname, gensym("setname"), A_DEFSYMBOL, A_DEFSYMBOL, 0);
    class_addanything(blocks_class, (t_method)blocks_command);
    class_addbang(blocks_class, (t_method)blocks_bang);
}

static void blocks_setname(t_blocks *x, t_symbol *serial, t_symbol *name) {
    x->juceThread->mBlockFinder->setPdNameForSerial(serial->s_name, name->s_name);
}

static void blocks_command(t_blocks *x, t_symbol *s, int argc, t_atom *argv) {
    if (argc>0) {
        x->juceThread->mBlockFinder->doBlockCommand(s, argc, argv);
    } else {
        error("too few arguments");
    }
}

static void blocks_bang(t_blocks *x) {
    x->juceThread->mBlockFinder->pollInfos();
}
