# CUE Charset documentation

CUE Charset reads external CUE sheets saved in legacy Cyrillic encodings. It converts their
text and filenames to UTF-8 in memory and delegates playback to foobar2000's audio services.

## Choose your starting point

| What you need | Where to go |
| --- | --- |
| Learn the development workflow through a small exercise | [Build and explore the encoding tests](tutorials/first-build.md) |
| Install the component or select an encoding | [Install and configure](how-to/install-and-configure.md) |
| Produce a tested component package | [Build, test, and package](how-to/build-and-test.md) |
| Fix unreadable text or a missing audio reference | [Troubleshoot a CUE sheet](how-to/troubleshoot.md) |
| Validate the component in the player | [Run the manual acceptance checks](how-to/verify-in-player.md) |
| Look up defaults, commands, limits, or APIs | [Reference](#reference) |
| Understand the design and its tradeoffs | [Explanation](#explanation) |

## Tutorials

A guided exercise for a first encounter with the project:

- [Build and explore the encoding tests](tutorials/first-build.md)

## How-to guides

Procedures for a specific task:

- [Install and configure](how-to/install-and-configure.md)
- [Build, test, and package](how-to/build-and-test.md)
- [Troubleshoot a CUE sheet](how-to/troubleshoot.md)
- [Run the manual acceptance checks](how-to/verify-in-player.md)

## Reference

Facts to consult while working:

- [Behavior, preferences, and limits](reference/behavior.md)
- [Build commands, options, and dependencies](reference/build.md)
- [Encoding API](reference/encoding-api.md)
- [SDK integration APIs and source map](reference/sdk-api.md)
- [Player acceptance matrix](reference/player-test-matrix.md)

## Explanation

Design context and reasons for the implementation:

- [Architecture and data flow](explanation/architecture.md)
- [Why detection uses a configured fallback](explanation/encoding-policy.md)
- [Why the component uses a redirecting input](explanation/sdk-integration.md)

## Validation status

Automated encoding, path, and SDK parser tests do not establish player integration. Installation,
handler priority, playback, seeking, and preference persistence still require the
[player acceptance matrix](reference/player-test-matrix.md). Unexecuted cases remain pending.

## Documentation approach

This documentation follows [Diátaxis](https://diataxis.fr/). Place guided learning in tutorials,
task procedures in how-to guides, exact facts in reference, and design rationale in explanation.
Link between these forms when readers need a different kind of help.

[Project README](../README.md) · [Changelog](../CHANGELOG.md)
