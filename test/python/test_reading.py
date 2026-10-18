from concurrent.futures import ThreadPoolExecutor
import gc
import plistlib
import struct

import mkd
from support import ASSETS, DICTIONARIES, DictionaryTest


class ReadingTests(DictionaryTest):
    def test_metadata_is_detached_and_products_are_discoverable(self):
        metadata = self.product.metadata
        metadata.display_name.en = 'An impostor lion'
        self.assertEqual(self.product.metadata.display_name.en, 'Birbs & Saberlion')
        available = mkd.DirectoryDictionarySource(DICTIONARIES).find_all_available()
        self.assertEqual({item.id for item in available}, {'Birbs', 'BirbKanji'})
        self.assertIsNone(self.product.find_content('a missing birb'))

    def test_sequence_iteration_keeps_the_order_alive(self):
        order = mkd.DictionaryProduct.open(DICTIONARIES / 'Birbs').dictionaries[0].entry_order
        self.assertEqual(order[-1], 2)
        self.assertEqual(order.index(1), 1)
        iterator = iter(order)
        del order
        gc.collect()
        self.assertEqual(list(iterator), [3, 1, 2])
        with self.assertRaises(IndexError):
            self.birbs.entry_order[3]
        with self.assertRaises(mkd.Error):
            self.birbs.entry_order.next(99)

    def test_buffer_view_survives_the_product_and_is_read_only(self):
        product = mkd.DictionaryProduct.open(DICTIONARIES / 'BirbKanji')
        buffer = product.dictionaries[0].entry_data(1)
        view = memoryview(buffer)
        del product, buffer
        gc.collect()
        self.assertTrue(view.readonly)
        self.assertIn('鳥語'.encode(), bytes(view))
        with self.assertRaises(TypeError):
            view[0] = 0

    def test_media_fonts_and_appendix_values_are_owned_safely(self):
        graphic = self.birbs.graphics.get('saberlion.png')
        self.assertEqual(bytes(graphic), (ASSETS / 'saberlion.png').read_bytes())
        self.assertTrue(bytes(self.birbs.resource('audio/chirp.wav')).startswith(b'RIFF'))
        font = self.birbs.fonts[0]
        self.assertEqual(font.filename, 'BirbMincho.ttf')
        output = self.workspace() / font.filename
        font.write_to(output)
        self.assertEqual(output.read_bytes(), (ASSETS / font.filename).read_bytes())
        entry = self.birbs.appendix_entry_lists[0].entry_at(1)
        del self.birbs, self.product
        gc.collect()
        self.assertEqual(entry.label, 'セイバーライオン')
        self.assertEqual(entry.entry_id, mkd.EntryId(3))

    def test_search_outlives_its_dictionary_and_results_are_sequences(self):
        search = mkd.DictionarySearch(mkd.DictionaryProduct.open(DICTIONARIES / 'Birbs').dictionaries[0])
        gc.collect()
        result = search.search('birb', match=mkd.SearchMode.EXACT)
        del search
        gc.collect()
        self.assertEqual(list(result), [mkd.EntryId(1), mkd.EntryId(2)])
        self.assertEqual(result[-1], mkd.EntryId(2))
        with self.assertRaises(IndexError):
            result[2]

    def test_queries_can_share_a_search_object_across_threads(self):
        search = mkd.DictionarySearch(self.birbs)
        with ThreadPoolExecutor(max_workers=2) as workers:
            results = list(workers.map(search.search, ('つぐみ', 'すずめ')))
        self.assertEqual([list(result) for result in results], [[mkd.EntryId(1)], [mkd.EntryId(2)]])

    def test_export_delivers_owned_progress_and_propagates_callback_errors(self):
        options = mkd.ExportOptions(resources=[mkd.ResourceType.CONTENTS])
        events = []
        result = mkd.export_to_directory(self.birbs, self.workspace() / 'export',
                                         options=options, progress=events.append)
        self.assertEqual((result.exported, result.failed), (3, 0))
        self.assertTrue(any(isinstance(event, mkd.ProgressEvent) for event in events))

        def hungry_lion(event):
            raise ValueError('がおー')

        with self.assertRaisesRegex(ValueError, 'がおー'):
            mkd.export_to_directory(self.birbs, self.workspace() / 'hungry',
                                    options=options, progress=hungry_lion)

    def test_information_and_license_pages_use_product_metadata(self):
        contents = DICTIONARIES / 'Birbs/Contents'
        menu = plistlib.loads((contents / 'More.plist').read_bytes())
        self.assertEqual(menu[0]['items'][0]['contents'], 'about.html')
        page = (contents / 'about.html').read_text(encoding='utf-8')
        self.assertIn('小鳥とセイバーライオン', page)
        self.assertIn('Original birb descriptions', page)
        self.assertTrue((contents / 'license/0.html').is_file())

    def test_icons_include_real_retina_scale_variants(self):
        icons = DICTIONARIES / 'Birbs/Contents/icons'
        for filename, size in (('Birbs-60.png', 60), ('Birbs-60@2x.png', 120), ('Birbs-60@3x.png', 180)):
            with self.subTest(filename=filename):
                data = (icons / filename).read_bytes()
                self.assertEqual(struct.unpack('>II', data[16:24]), (size, size))

    def test_appendix_menu_preserves_submenus_and_retains_resources(self):
        menu = self.product.appendices
        section = menu.sections[0]
        self.assertEqual(section.name, '小鳥と獅子')
        self.assertEqual([item.name for item in section.items], ['仲間たち', 'もっと見る'])
        entries = self.product.open_appendix_list(section.items[0].target)
        self.assertEqual(entries.entry_at(1).dictionary_id, 1000)
        submenu = section.items[1].target
        link, document, gallery = [item.target for item in submenu.sections[0].items]
        owner = self.product.find_content(link.dictionary_id)
        self.assertIn(str(link.entry_id), owner.entry_xml(link.entry_id.page_id))
        self.assertEqual(document.fragment, 'lion')
        data = mkd.DictionaryProduct.open(DICTIONARIES / 'Birbs').open_appendix_document(document)
        image = self.product.resource(gallery.images[0])
        del self.product, menu, section, submenu
        gc.collect()
        self.assertEqual(document.path, 'birbs/guide.html')
        self.assertIn('hungry lion', data.utf8())
        self.assertEqual(bytes(image), (ASSETS / 'saberlion.png').read_bytes())
