# Unreal Log Parser

Compact, fast and easy to use Unreal Engine targeted Log Parser. Parses every log line with targeted verbosity into:

Original line: line
Count: line count
Verbosity: line verbosity level (e.g. Error)
Category: log category (e.g. LogAbilitySystem)
and sorts them in descending order. It also have some useful arguments:

-t enables telemetry
-v [error|warning|display] allows to specify minimum verbosity level to be included in the parsing process
-p [path to logs to parse] allows to specify path to the folder containing logs to parse
-r [path to create result .txt] allows to specify path that will be used to create result .txt in
I'm planning to add more features, make the result prettier or more useful if needed and make it multithreaded 👀
