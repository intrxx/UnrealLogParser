# Unreal Log Parser

Compact, fast and easy to use Unreal Engine targeted Log Parser.

## Details

Parses every log line with targeted verbosity into: 
- Original line: **normalized log line**
- Count: **line count**
- Verbosity: **line verbosity level (e.g. Error)**
- Category: **log category (e.g. LogAbilitySystem)**
and sorts them in descending order.

```
Original line: LogAbilitySystem: Warning: No GameplayCueNotifyPaths were specified in DefaultGame.ini under [/Script/GameplayAbilities.AbilitySystemGlobals]. Falling back to using all of /Game/. This may be slow on large projects. Consider specifying which paths are to be searched.
Count: 2
Verbosity:  Warning
Category: LogAbilitySystem
```

It also have some useful parameters:
- `-t` enables telemetry
- `-v [error|warning|display]` allows to specify minimal verbosity level to be included in the parsing process
- `-p [path to logs to parse]` allows to specify path to the folder containing logs to parse
- `-r [path to create result .txt]` allows to specify path that will be used to create result .txt in

## Usage

To use the parser you need to place your log files into the `LogsToParse` directory located at your program root or use it with a `-p` parameter to specify the path to the folder with logs to parse, e.g `LogParser.exe -p D:\MyUnrealGameProject\Build\Windows\MyGame\Saved\Logs`.

I'm planning to add more features, make the result prettier or more useful if needed and make it multithreaded 👀
