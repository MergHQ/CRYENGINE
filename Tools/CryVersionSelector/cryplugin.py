#!/usr/bin/env python3

import cryregistry
import json
import os.path


class CryPlugin:
    def __init__(self):
        self.data = {}
        self.path = ''

    def load(self, path):
        if not path:
            raise Exception('No path specified')

        file = open(path, 'r', encoding="utf-8")
        self.data = json.loads(file.read())
        file.close()

        self.path = path

    def binaries(self, config_path):
        binaries = self.data.get('content', {}).get('binaries', [])

        # Get a list of all files in the directory if nothing was specified
        if not binaries:
            plugin_root = os.path.dirname(self.path)
            binary_directory = os.path.join(plugin_root, config_path)
            filenames = os.listdir(binary_directory)

            binaries = []
            for it in filenames:
                binaries.append(os.path.join(binary_directory, it))

        return binaries

    def dependencies(self):
        return self.data.get('require', {}).get('plugins')

    def isNative(self):
        return self.data.get('info', {}).get('type') == 'native'

    def guid(self):
        return self.data.get('info', {}).get('guid')

    def name(self):
        return self.data.get('info', {}).get('display_name')


def find(engine_id, plugin_guid):
    engine_registry = cryregistry.load_engines()
    engine = engine_registry.get(engine_id, {})
    plugin = engine.get('plugins', {}).get(plugin_guid, {})

    return plugin.get('uri')
