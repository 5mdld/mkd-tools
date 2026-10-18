import array
import gc
import xml.etree.ElementTree as ET

import mkd
from support import DictionaryTest, new_dictionary, new_product, one_birb


class CreationTests(DictionaryTest):
    def test_minimal_authoring_publishes_a_searchable_dictionary(self):
        path = self.workspace() / 'Birbs'
        product = new_product(path)
        dictionary = new_dictionary(product, encrypted=True)
        target = one_birb(dictionary)
        dictionary.finish()
        self.assertFalse(path.exists())
        product.finish()
        reopened = mkd.DictionaryProduct.open(path).dictionaries[0]
        self.assertEqual(list(mkd.DictionarySearch(reopened).search('すずめ')), [target])
        self.assertEqual(list(reopened.entry_order), [target.page_id])
        self.assertIsNone(reopened.entry_order.next(target.page_id))

    def test_entry_handles_keep_the_writer_alive(self):
        path = self.workspace() / 'Birbs'
        product = new_product(path)
        dictionary = new_dictionary(product)
        entry = dictionary.create_entry()
        del product, dictionary
        gc.collect()
        self.assertEqual(entry.target, mkd.EntryId(1))
        entry.write_xml('<html><body>A birb waiting to hatch.</body></html>')
        del entry
        gc.collect()
        self.assertFalse(path.exists())

    def test_allocated_targets_connect_xml_headlines_and_search_keys(self):
        path = self.workspace() / 'Birbs'
        product = new_product(path)
        dictionary = new_dictionary(product)
        entry = dictionary.create_entry()
        idiom = entry.add_target(mkd.EntryTargetKind.IDIOM)
        entry.write_xml(f'<html><body><p id="{idiom}">一石二鳥</p></body></html>')
        dictionary.add_headline(mkd.HeadlineComponents('鳥', entry.target))
        dictionary.add_headline(mkd.HeadlineComponents('一石二鳥', idiom))
        dictionary.add_search_key(mkd.SearchScope.HEADWORD, 'とり', entry.target)
        dictionary.add_search_key(mkd.SearchScope.IDIOM, 'いっせきにちょう', idiom)
        dictionary.finish()
        product.finish()
        reopened = mkd.DictionaryProduct.open(path).dictionaries[0]
        self.assertEqual(str(idiom), '00001-4001')
        self.assertEqual(list(mkd.DictionarySearch(reopened).search(
            'いっせきにちょう', scope=mkd.SearchScope.IDIOM)), [idiom])
        self.assertIsNotNone(ET.fromstring(reopened.entry_xml(idiom.page_id)).find(f'.//*[@id="{idiom}"]'))

    def test_borrowed_input_bytes_are_consumed_during_the_call(self):
        path = self.workspace() / 'Birbs'
        product = new_product(path)
        dictionary = new_dictionary(product)
        one_birb(dictionary)
        data = bytearray(b'a sleepy birb')
        dictionary.add_graphic('perch.txt', memoryview(data))
        data[:] = b'x' * len(data)
        dictionary.finish()
        product.finish()
        reopened = mkd.DictionaryProduct.open(path).dictionaries[0]
        self.assertEqual(bytes(reopened.graphics.get('perch.txt')), b'a sleepy birb')

    def test_noncontiguous_and_nonbyte_buffers_are_rejected(self):
        product = new_product(self.workspace() / 'Birbs')
        dictionary = new_dictionary(product)
        for data in (memoryview(b'birbs')[::2], array.array('I', [1, 2])):
            with self.assertRaises((TypeError, BufferError)):
                dictionary.add_graphic('perch.txt', data)

    def test_inline_content_encodes_ruby_links_and_xml_escaping(self):
        text = mkd.InlineText()
        text.append(mkd.Ruby('雀', 'すずめ'))
        text.append(' & ')
        text.append(mkd.EntryLink(mkd.EntryId(3), 'セイバーライオン'))
        self.assertEqual(text.plain_text(), '雀 & セイバーライオン')
        self.assertIn('&amp;', text.to_xml())
        self.assertEqual(mkd.InlineText.from_headline(text.to_headline()).to_xml(), text.to_xml())

    def test_cyclic_and_escaping_appendix_menus_are_rejected(self):
        import plistlib
        import shutil
        from support import DICTIONARIES

        for name, contents in (('cycle', 'Appendix.plist'), ('escape', '../outside.html')):
            path = self.workspace() / 'Birbs'
            shutil.copytree(DICTIONARIES / 'Birbs', path)
            menu = [{'children': [{'name': 'Lost birb', 'contents': contents}]}]
            (path / 'Contents/Appendix.plist').write_bytes(plistlib.dumps(menu))
            with self.assertRaises(mkd.Error):
                mkd.DictionaryProduct.load_metadata(path)
