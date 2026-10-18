import os
from pathlib import Path
import sys
import tempfile
import unittest

import mkd

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'dictionary'))
from common import new_dictionary, new_product, one_birb


DICTIONARIES = Path(os.environ['MKD_TEST_DICTIONARIES'])
ASSETS = Path(os.environ['MKD_TEST_ASSETS'])


class DictionaryTest(unittest.TestCase):
    def setUp(self):
        self.product = mkd.DictionaryProduct.open(DICTIONARIES / 'Birbs')
        self.birbs = self.product.dictionaries[0]

    def workspace(self):
        temporary = tempfile.TemporaryDirectory(prefix='mkd-birbs-')
        self.addCleanup(temporary.cleanup)
        return Path(temporary.name)
