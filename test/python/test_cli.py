import os
import subprocess

from support import DICTIONARIES, DictionaryTest


class CliTests(DictionaryTest):
    def run_cli(self, *arguments):
        return subprocess.run([os.environ['MKD_TEST_CLI'], '-d', str(DICTIONARIES), *arguments],
                              capture_output=True, text=True, encoding='utf-8')

    def test_lists_and_describes_the_small_dictionary(self):
        listing = self.run_cli('list')
        self.assertEqual(listing.returncode, 0, listing.stderr)
        self.assertIn('Birbs', listing.stdout)
        info = self.run_cli('info', 'Birbs')
        self.assertEqual(info.returncode, 0, info.stderr)
        self.assertIn('test.birbs', info.stdout)

    def test_exports_selected_entries_without_a_terminal(self):
        output = self.workspace() / 'export'
        result = self.run_cli('export', 'Birbs', '-o', str(output), '--only', 'entries', '--pretty')
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(len(list(output.rglob('*.xml'))), 3)
        self.assertFalse(list(output.rglob('*.png')))

    def test_help_and_invalid_invocations(self):
        self.assertEqual(self.run_cli('--help').returncode, 0)
        self.assertNotEqual(self.run_cli('export', 'MissingBirb').returncode, 0)
        self.assertNotEqual(self.run_cli('export', 'Birbs', '--only', 'lion-food').returncode, 0)
