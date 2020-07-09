This is the `xinp` tool for extracting files backed up using the Dell Backup And Recovery files (`*.inp`).

It was born out of the frustration arising from having used the DBaR software to create a backup before switching to Linux. Getting the DBaR tool to run or finding the old MigRestore software by Dell and getting that to run can be a daunting task on Windows already, nevermind trying to set it up with Wine or a VM.

Therefore, I reverse-engineered the `inp` file format and wrote a tool to extract the files. Only full backups are supported at the moment, and the binary was compiled on Linux. It extracts the files having as name the full path of the file, so if the path is longer than the maximum allowed filename length, it will give an error.

You can find a description of the file format [here](inp.md).

Usage: `xinp file...`

Moral of the story: never rely on proprietary/manufacturer-specific tools for backing up your files!
