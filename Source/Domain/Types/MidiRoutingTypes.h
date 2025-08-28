/*
  ==============================================================================

    MidiRoutingTypes.h
    Shared types for MIDI routing functionality

  ==============================================================================
*/

#pragma once

namespace HAM {

//==============================================================================
/**
 * MIDI routing mode determines where track MIDI is sent
 */
enum class MidiRoutingMode
{
    PLUGIN_ONLY,    // Send MIDI only to internal plugins
    EXTERNAL_ONLY,  // Send MIDI only to external MIDI devices
    BOTH           // Send MIDI to both plugins and external devices
};

} // namespace HAM