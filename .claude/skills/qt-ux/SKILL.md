---
name: qt-ux
description: Design and build the fin-bs desktop interface in Qt Quick (QML) as a UX specialist would - what the window shows, how fields explain themselves, how a run reports progress, how the window is tested without a display, and how screenshots and demos are produced. Use when changing anything under source/gui, source/app or their QML.
---

# Qt Quick interface for fin-bs

## Why these rules

The person reading this window will trade real money on what it says. Every rule below exists
to make a number legible, a refusal explicit, and a wait honest.

## Architecture, in one paragraph

The presentation layer under `source/present` is Qt-free and holds the truth: the scenario
document, the run controller, the formatted result tables and the chart geometry. The Qt layer
under `source/gui` only exposes that truth to QML: one `QObject` model per concept (`ScenarioModel`,
`UnderlyingsModel`, `RunModel`, `ResultTableModel`, `ChartSeries`), a painted item for the fan chart
(`FanChartItem`), and a `Workspace` that owns them and the file boundary. QML binds to properties and
calls invokables; it never computes a financial figure. A number shown in words (euro, percent) is a
hint next to the stored figure (cents, basis points), never a second source of truth.

## Layout and flow

- Left pane: the scenario, grouped the way a person thinks about it, in this order: capital and
  horizon; strategy levers; wealth tax and simulation (collapsible, rarely touched); underlyings.
- Right pane: results. Highlight cards first (the handful of figures that matter), then tabs for the
  summary table, the fan chart, the cycle table and the ledger. The disclaimer sits under every
  result and is never hidden.
- Top bar: file actions on the left, the scenario name in the middle, the three runs and *Cancel*
  on the right, the theme switch last. The bar keeps its own colours in both themes.
- Bottom bar: one line of status (the model's refusal, a file complaint, or the run state), and a
  progress bar only while a run is under way.

## Fields

- Every field is shown in the unit the model stores (cents, basis points, days) with the unit
  written beside it, and the same value in words underneath (`= EUR 200,000.00`, `= 30.00 %`).
- The window never clamps or rounds a value. A value the document refuses turns the field red and
  puts the document's own sentence (which names the field) in a banner and in the status line; the
  run buttons disable. The sentence comes from the presentation layer, not from QML.
- Object names on controls match the scenario file's keys (`capital_cents`,
  `target_delta_basis_points`), so a test or a tour can find a control by the name a user would
  look for in the file.
- Spin boxes step in sensible units (100 basis points for a delta, 25 for a rate); text fields
  commit on editing finished, not on every keystroke.

## Runs and waiting

- A run never blocks the window: it runs on a worker thread and the model polls it at a steady
  beat. While it runs: the buttons that start a run disable, *Cancel* enables, the progress bar
  shows completed/total, and the status line says "Running".
- Cancelling keeps what had been run and says so. A failure shows the controller's own message.
- Results are cleared when a run starts, so a stale table is never shown beside a new progress bar.

## Theme and style

- Qt Quick Controls, Material style, Indigo primary and accent; light by default, dark on a switch.
- The chart item takes its background and ink from the theme, so it reads in both.
- Tables use a monospaced face for figures and right-align numbers; headings come from the
  presentation layer's names.

## Compatibility

- Code must build with Qt 6.4 (the apt packages used for screenshots here) and with Qt 6.8 (CI).
  Avoid QML types and properties newer than 6.4 (for example `TableViewDelegate`, `SelectionRectangle`
  on `TableView`, `editTriggers`). Qt Quick Controls `HorizontalHeaderView`, `SplitView`, `TabBar`,
  `StackLayout` and `QtQuick.Dialogs.FileDialog` are fine.
- A `Flickable` (including `ScrollView` and `TableView`) adopts its children: overlay labels live
  beside the layout, not inside the view.
- A `ScrollView` has a `locale` property; a custom property on it must not reuse that name.
- `QQuickStyle::setStyle` is called once per process, before the first QML load.

## Testing without a display

- Tests are Qt Test binaries registered with `emil_add_test`, run under `QT_QPA_PLATFORM=offscreen`
  and `QT_QUICK_BACKEND=software` (set in `source/gui/test/CMakeLists.txt`).
- Model tests need no QML: they drive `ScenarioModel`, `Workspace` and `RunModel` directly and keep
  the golden projection check.
- The window test loads the embedded `Main.qml` through a real engine, finds controls by object
  name, presses *Project* through its `clicked` signal and reads bindings back.
- The chart test paints the item into an image and asserts one pixel of the median.

## Screenshots and demos

- The application takes a scripted tour when `FIN_BS_DEMO=<folder>` is set, with
  `FIN_BS_DEMO_SCENARIO=<file>` choosing the scenario (default: the BENELUX scenario). Run it as
  `QT_QPA_PLATFORM=offscreen QT_QUICK_BACKEND=software FIN_BS_DEMO=/tmp/shots fin.app`; it writes
  one numbered PNG per step (scenario, projection under way and done, fan chart, cycles, ledger,
  simulation under way and done, underlyings, a validation refusal, the dark theme) and quits.
- An animated tour: `ffmpeg -framerate 0.6 -pattern_type glob -i '*.png' -vf "scale=1080:-1,split[a][b];[a]palettegen[p];[b][p]paletteuse" tour.gif`.
- Committed pictures live under `documentation/screenshots/`; refresh them when the layout changes.
