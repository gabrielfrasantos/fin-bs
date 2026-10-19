#pragma once

namespace fin::gui
{
    // Registers the QML types and the embedded files. Called once, before an engine loads the
    // window; harmless when called again.
    void Initialise();
}
