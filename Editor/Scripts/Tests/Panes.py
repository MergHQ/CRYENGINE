# Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

import unittest
import random

class TestPanes(unittest.TestCase):
    panes = general.get_pane_class_names()

    def test(self):        
        self.run_test()
        general.open_level_no_prompt("gamesdk/Levels/_TestMaps/Sandbox_Tests/Sandbox_Tests.cry")
        self.run_test()
        
    def run_test(self):
        random.seed(1234)
        for _ in range(30):
            openPanes = random.sample(self.panes, int(len(self.panes) / 2))            
            for pane in openPanes:
                general.open_pane(pane)
                general.idle_wait(0.05)
            closePanes = random.sample(self.panes, int(len(self.panes) / 2))
            for pane in closePanes:
                general.close_pane(pane)
                general.idle_wait(0.05)
                
unittest.main(exit = False, verbosity = 2, defaultTest="TestPanes")