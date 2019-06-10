# KCD Quest Timeout Patcher
Patches "ObjectiveTimer::IsTimedOut" (not a real name) function in WHGame.dll to always return false. In other words, it simply makes a game to ignore "autocomplete_time" and "expiration_time" timers set for quest objectives in "Data\Tables.pak\Libs\Tables\quest\quest_objective".

It will NOT prevent quests from failling if they are scripted to fail.
For example, quest "q_ratajTournament" will fail if you won't attend it in time.
This has nothing to do with "autocomplete_time" timer, so expect some quests to still fail.

But in general it should fix issues with quests like "q_waldensians" when it fails while you skip time.

### IMPORTANT
It will break quests that use these timers to set a quest stage (e.g. "timerHenryMustRead" objective in "q_plagueInMerhojed").
So use this in cases when you don't have a time left to complete certain quest objectives and you don't want to reload a previous save game. Apply patch, complete the objective\quest and then remove patch to get everything back to normal.

### Compilation
Compilation is done as a single unit, just compile main.cpp and it should work.
For more details see tools/build.bat file.

### Usage
This is a command-line program, so you have to run it from a "cmd.exe".
(http://lmgtfy.com/?q=how+to+run+a+command+line+program+on+windows)

kcd_quest_timeout_patcher.exe <command> <filepath>
        <command> - patch|unpatch
        <filepath> - a path to WHGame.dll

To patch WHGame.dll use "patch" command.
To unpatch WHGame.dll use "unpatch" command.

### How to patch
1) Download "kcd_quest_timeout_patcher.exe" to a folder you like (e.g. c:\users\adam\downloads).
2) Navigate to that folder and copy a filepath to it (e.g. c:\users\adam\downloads).
3) Run "cmd.exe" (use a "run" command and type in "cmd.exe" then press enter, you can launch "run" by pressing win+r or by searching for it in a start menu). A window with text in it should appear.
4) Type in cd /d "<filepath>" replacing <filepath> with a filepath you've copied in step 2 (e.g. cd /d "c:\users\adam\downloads") and press enter. Make sure to not forget double quotes, especially if a filepath contains spaces.
5) Type in kcd_quest_timeout_patcher.exe <command> "<filepath to whgame.dll>" replacing <command> with either patch or unpatch word, and <filepath to whgame.dll> with a filepath to WHGame.dll (e.g. kcd_quest_timeout_patcher.exe patch "D:\Games\Kingdom Come Deliverance\bin\Win64\WHGame.dll") and press enter. Make sure to not forget double quotes, especially if a filepath contains spaces.
6) If everything was done correcly then you should see a message "Successfully patched" or "Successfully unpatched", if not, then it will show an error message. If you get "Function signatures doesn't match" error message then it's either already patched\unpatched or something is wrong with a file (not WHGame.dll or a different version of it that program cannot "understand").

### Supported versions
Should work on most game versions (create an issue if it doesn't and specify problematic version).
List of versions that were tested is located below.

### Tested versions
* 1.8.2
* 1.9.0
* 1.9.1 (CC)
