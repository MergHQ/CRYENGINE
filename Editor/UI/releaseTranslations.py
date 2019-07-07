# Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

import os

os.chdir('qml')
startDir = os.getcwd()

# since it's a .exe file it will only work on windows, but we may as well
# construct the path in a platform-independent way.
lreleaseCmd = os.path.join(startDir, '..', '..', '..',
                          'Code', 'SDKs', 'Qt', 'x64', 'bin', 'lrelease.exe ')

print(startDir)

# Korean, Japanese and Simplified Chinese
targetLanguages = ['ko', 'ja', 'zh_CN']

for lang in targetLanguages:
    os.chdir(startDir)
    tgtLang = '-target-language ' + lang
    os.system(lreleaseCmd + 'this_' + lang + '.ts')

    for fileName in os.listdir():
        if not fileName.endswith(".ts"):
            continue

        os.system(lreleaseCmd + ' ' + fileName)

        print(('Finished processing: ' + fileName))
