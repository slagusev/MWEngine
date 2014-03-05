/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2013-2014 Igor Zinken - http://www.igorski.nl
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include "delay.h"
#include "global.h"
#include "utils.h"
#include <math.h>

/* constructor / destructor */

/**
 * @param aDelayTime    {float} in milliseconds, time between consecutive repeats
 * @param aMaxDelayTime {float} in milliseconds, the maximum value we're likely to expect
 *                               in case delay time will fluctuate in the application
 * @param aMix          {float} 0-1, percentage of dry/wet mix
 * @param aFeedback     {float} 0-1, amount of repeats
 * @param amountOfChannels {int} amount of output channels
 */
Delay::Delay( float aDelayTime, float aMaxDelayTime, float aMix, float aFeedback, int amountOfChannels )
{
    _time        = ( int ) round(( audio_engine::SAMPLE_RATE * .001 ) * aDelayTime );
    _maxTime     = ( int ) round(( audio_engine::SAMPLE_RATE * .001 ) * aMaxDelayTime );
    _delayBuffer = new AudioBuffer( amountOfChannels, _maxTime );
    _mix         = aMix;
    _feedback    = aFeedback;
    _delayIndices = new int[ amountOfChannels ];

    for ( int i = 0; i < amountOfChannels; ++i )
        _delayIndices[ i ] = 0;

    _amountOfChannels = amountOfChannels;
}

Delay::~Delay()
{
    delete _delayBuffer;
    delete[] _delayIndices;
}

/* public methods */

void Delay::apply( AudioBuffer* sampleBuffer, bool isMonoSource )
{
    float delay;
    int readIndex, delayIndex;

    int bufferSize = sampleBuffer->bufferSize;

    for ( int c = 0, ca = sampleBuffer->amountOfChannels; c < ca; ++c )
    {
        SAMPLE_TYPE* channelBuffer = sampleBuffer->getBufferForChannel( c );
        SAMPLE_TYPE* delayBuffer   = _delayBuffer->getBufferForChannel( c );
        delayIndex                 = _delayIndices[ c ];

        for ( int i  = 0; i < bufferSize; ++i )
        {
            readIndex = delayIndex - _time + 1;

            if ( readIndex < 0 )
                readIndex += _time;

            // read the previously delayed samples from the buffer
            // ( for feedback purposes ) and append the current sample to it

            delay = delayBuffer[ readIndex ];

            delayBuffer[ delayIndex ] = channelBuffer[ i ] + delay * _feedback;

            if ( ++delayIndex == _time )
                delayIndex = 0;

            // higher feedback levels can cause a massive noise-fest, "limit" them!
            if ( _feedback > .5f )
                channelBuffer[ i ] += ( delay * _mix * ( 1.5f - _feedback ));
            else
                channelBuffer[ i ] += ( delay * _mix );
        }
        _delayIndices[ c ]  = delayIndex; // update last index

        // omit unnecessary cycles by copying the mono content
        // TODO: make delay stereo multi-tap ! ;)
        if ( isMonoSource )
        {
            sampleBuffer->applyMonoSource();
            break;
        }
    }
}

/**
 * clears existing buffer contents
 */
void Delay::reset()
{
    if ( _delayBuffer != 0 )
        _delayBuffer->silenceBuffers();
}

/* getters / setters */

float Delay::getDelayTime()
{
    return _time / ( audio_engine::SAMPLE_RATE * .001 );
}

void Delay::setDelayTime( float aValue )
{
    _time = ( int ) round(( audio_engine::SAMPLE_RATE * .001 ) * aValue );

    for ( int i = 0; i < _amountOfChannels; ++i )
    {
        if ( _delayIndices[ i ] > _time )
            _delayIndices[ i ] = 0;
    }
}

float Delay::getMix()
{
    return _mix;
}

void Delay::setMix( float aValue )
{
    _mix = aValue;
}

float Delay::getFeedback()
{
    return _feedback;
}

void Delay::setFeedback( float aValue )
{
    _feedback = aValue;
}
