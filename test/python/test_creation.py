import array
import gc
import xml.etree.ElementTree as ET

import mkd
from support import DictionaryTest, new_dictionary, new_product, one_birb


class CreationTests(DictionaryTest):
    def test_preflight_is_bounded_deterministic_and_keeps_distinct_source_origins(self):
        reports = []
        for pages in ((1, 2, 3, 4), (4, 3, 2, 1)):
            product = new_product(self.workspace() / 'Origins')
            dictionary = new_dictionary(product)
            for page in pages:
                source = mkd.SourceContext(filename='birds.xml', record=str(page))
                entry = dictionary.create_entry(page_id=page, source=source)
                entry.write_xml('<html>\n<body><img src="missing.png"/><img src="missing.png"/></body></html>')
                dictionary.add_headline(mkd.HeadlineComponents('鳥', entry.target))
                dictionary.add_search_key(mkd.SearchScope.HEADWORD, 'bird', entry.target)
            report = dictionary.preflight()
            self.assertIsInstance(report.issues, tuple)
            issue, = report.issues
            self.assertEqual(issue.code, 'missing-resource')
            self.assertEqual(issue.occurrences, 8)
            self.assertIsInstance(issue.origins, tuple)
            self.assertEqual(len(issue.origins), 3)
            self.assertEqual(issue.origins[0].context.filename, 'birds.xml')
            self.assertEqual(issue.origins[0].location.line, 2)
            with self.assertRaises(AttributeError):
                issue.code = 'changed'
            reports.append(report.describe())
            limited = dictionary.preflight(options=mkd.ValidationOptions(max_issues=0))
            self.assertEqual(limited.issues, ())
            self.assertFalse(limited.valid)
            self.assertTrue(limited.truncated)
            with self.assertRaises(mkd.Error):
                dictionary.finish()
            dictionary.add_asset('missing.png', b'')
            dictionary.finish()
            product.finish()
            self.assertFalse(report.valid)
        self.assertEqual(*reports)

    def test_kanwa_passage_keys_search_their_normalized_phrases(self):
        path = self.workspace() / 'Passages'
        product = new_product(path, mkd.CreationProfile.KANWA)
        dictionary = new_dictionary(product)
        entry = dictionary.create_entry()
        sense = entry.add_target(mkd.EntryTargetKind.SENSE)
        example = entry.add_target(mkd.EntryTargetKind.EXAMPLE)
        dictionary.add_headline(mkd.HeadlineComponents('鳥', entry.target))
        dictionary.add_headline(mkd.HeadlineComponents('鳥の意味', sense))
        dictionary.add_headline(mkd.HeadlineComponents('鳥の用例', example))
        dictionary.add_search_key(mkd.SearchScope.HEADWORD, '鳥', entry.target)
        dictionary.add_search_key(mkd.SearchScope.SENSE, '鳥などが声を出す', sense)
        entry.write_xml(f'<html><body><p id="{sense}">意味</p><p id="{example}">用例</p></body></html>')
        for term in ('鳥などが声を出す', '鳴禽', '鳴禽'):
            dictionary.add_search_key(mkd.SearchScope.SENSE, term, sense)
        dictionary.add_search_key(mkd.SearchScope.EXAMPLE, '春の林で鳴禽の声を聞く。', example)
        dictionary.finish()
        product.finish()
        reopened = mkd.DictionaryProduct.open(path).dictionaries[0]
        search = mkd.DictionarySearch(reopened)
        for scope, term, target in (
            (mkd.SearchScope.SENSE, '鳥などが声を出す', sense),
            (mkd.SearchScope.SENSE, '鳴禽', sense),
            (mkd.SearchScope.EXAMPLE, '春の林で鳴禽の声を聞く。', example),
        ):
            for policy in (mkd.SearchFallback.NONE, mkd.SearchFallback.DICTIONARY_RULES):
                self.assertEqual(list(search.search(term, scope=scope, fallback=policy,
                                                   match=mkd.SearchMode.EXACT)), [target])
        for term in ('声を', '声ヲ', '声 鳥', '声／鳥'):
            self.assertEqual(list(search.search(term, scope=mkd.SearchScope.SENSE)), [sense])
        for term in ('鳥声', '出す鳴禽', '聞く春'):
            self.assertEqual(list(search.search(term, scope=mkd.SearchScope.SENSE)), [])
        self.assertEqual(list(search.search('鳥', scope=mkd.SearchScope.SENSE,
                                            fallback=mkd.SearchFallback.NONE,
                                            match=mkd.SearchMode.EXACT)), [sense])
        xml = ET.fromstring(reopened.entry_xml(entry.target.page_id))
        for target in (sense, example):
            companion = mkd.EntryId(target.page_id, target.item_id + 0x800)
            self.assertIsNone(xml.find(f'.//*[@id="{companion}"]'))

    def test_exact_only_passage_keys_retain_raw_exact_lookup(self):
        for profile in (mkd.CreationProfile.GENERAL, mkd.CreationProfile.KANWA):
            path = self.workspace() / 'Exact'
            product = new_product(path, profile)
            dictionary = new_dictionary(product)
            target = one_birb(dictionary)
            for scope in (mkd.SearchScope.SENSE, mkd.SearchScope.EXAMPLE):
                for term in ('鳥', '鳴禽'):
                    dictionary.add_search_key(scope, term, target, exact_only=True)
            dictionary.finish()
            product.finish()
            reopened = mkd.DictionaryProduct.open(path).dictionaries[0]
            search = mkd.DictionarySearch(reopened)
            for scope in (mkd.SearchScope.SENSE, mkd.SearchScope.EXAMPLE):
                for term in ('鳥', '鳴禽'):
                    self.assertEqual(list(search.search(term, scope=scope, match=mkd.SearchMode.EXACT,
                                                       fallback=mkd.SearchFallback.NONE)), [target])
                    self.assertEqual(list(search.search(term, scope=scope,
                                                       fallback=mkd.SearchFallback.NONE)), [])
                self.assertEqual(list(search.search('鳴', scope=scope)), [])

    def test_passage_components_require_consistent_key_options(self):
        for exact_first in (True, False):
            product = new_product(self.workspace() / 'Options', mkd.CreationProfile.KANWA)
            dictionary = new_dictionary(product)
            target = dictionary.create_entry().target
            calls = [('声', True), ('鳥声', False)]
            if not exact_first:
                calls.reverse()
            dictionary.add_search_key(mkd.SearchScope.SENSE, calls[0][0], target, exact_only=calls[0][1])
            with self.assertRaisesRegex(mkd.Error, "Search key.*sense.*Conflicting options"):
                dictionary.add_search_key(mkd.SearchScope.SENSE, calls[1][0], target, exact_only=calls[1][1])

    def test_sense_and_example_indexes_have_distinct_targets(self):
        for scopes in ((mkd.SearchScope.SENSE, mkd.SearchScope.EXAMPLE),
                       (mkd.SearchScope.EXAMPLE, mkd.SearchScope.SENSE)):
            product = new_product(self.workspace() / 'Scopes', mkd.CreationProfile.KANWA)
            dictionary = new_dictionary(product)
            target = dictionary.create_entry().target
            dictionary.add_search_key(scopes[0], '鳥声', target)
            with self.assertRaisesRegex(mkd.Error, 'distinct targets'):
                dictionary.add_search_key(scopes[1], '声鳥', target)

    def test_kanwa_passage_companions_cannot_replace_real_anchors(self):
        for keys_first in (True, False):
            product = new_product(self.workspace() / 'Collision', mkd.CreationProfile.KANWA)
            dictionary = new_dictionary(product)
            entry = dictionary.create_entry()
            target = entry.add_target(mkd.EntryTargetKind.SENSE)
            companion = mkd.EntryId(target.page_id, target.item_id + 0x800)
            dictionary.add_headline(mkd.HeadlineComponents('鳥', entry.target))
            dictionary.add_headline(mkd.HeadlineComponents('鳥声', target))
            dictionary.add_search_key(mkd.SearchScope.HEADWORD, '鳥', entry.target)
            if keys_first:
                dictionary.add_search_key(mkd.SearchScope.SENSE, '鳥声', target)
            entry.write_xml(f'<html><body><p id="{target}">鳥声</p><span id="{companion}"/></body></html>')
            if not keys_first:
                dictionary.add_search_key(mkd.SearchScope.SENSE, '鳥声', target)
            report = dictionary.preflight()
            self.assertIn('passage-companion-collision', [issue.code for issue in report.issues])
            with self.assertRaisesRegex(mkd.Error, 'reserves ID.*automatically generated search record'):
                dictionary.finish()

    def test_passage_search_record_ids_must_fit(self):
        product = new_product(self.workspace() / 'Overflow', mkd.CreationProfile.KANWA)
        dictionary = new_dictionary(product)
        with self.assertRaisesRegex(mkd.Error, 'companion overflows'):
            dictionary.add_search_key(mkd.SearchScope.SENSE, '鳥声', mkd.EntryId(1, 0xF800))

    def test_last_explicit_passage_target_has_a_searchable_companion(self):
        path = self.workspace() / 'LastPassage'
        product = new_product(path, mkd.CreationProfile.KANWA)
        dictionary = new_dictionary(product)
        target = mkd.EntryId(1, 0xF7FF)
        dictionary.add_page(1, f'<html><body><p id="{target}">鳥声</p></body></html>')
        dictionary.add_headline(mkd.HeadlineComponents('鳥', mkd.EntryId(1)))
        dictionary.add_headline(mkd.HeadlineComponents('鳥声', target))
        dictionary.add_search_key(mkd.SearchScope.HEADWORD, '鳥', mkd.EntryId(1))
        dictionary.add_search_key(mkd.SearchScope.SENSE, '鳥声', target)
        dictionary.finish()
        product.finish()
        reopened = mkd.DictionaryProduct.open(path).dictionaries[0]
        self.assertEqual(list(mkd.DictionarySearch(reopened).search('鳥声', scope=mkd.SearchScope.SENSE)), [target])

    def test_kanwa_target_capacity_reserves_companion_space(self):
        for kind, last in ((mkd.EntryTargetKind.SENSE, 0xF7FF), (mkd.EntryTargetKind.EXAMPLE, 0x57FF)):
            product = new_product(self.workspace() / 'Capacity', mkd.CreationProfile.KANWA)
            entry = new_dictionary(product).create_entry()
            for _ in range(2047):
                target = entry.add_target(kind)
            self.assertEqual(target.item_id, last)
            with self.assertRaisesRegex(mkd.Error, 'range exhausted'):
                entry.add_target(kind)

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

    def test_navigation_round_trip_keeps_subentry_targets(self):
        path = self.workspace() / 'Birbs'
        product = new_product(path)
        dictionary = new_dictionary(product)
        entry = dictionary.create_entry()
        compound = entry.add_target(mkd.EntryTargetKind.COMPOUND)
        entry.write_xml(f'<html><body><p id="{compound}">雀躍</p></body></html>')
        dictionary.add_headline(mkd.HeadlineComponents('雀', entry.target))
        dictionary.add_headline(mkd.HeadlineComponents('雀躍', compound))
        dictionary.add_search_key(mkd.SearchScope.HEADWORD, 'すずめ', entry.target)
        product.set_navigation(mkd.NavigationIndex(sections=[
            mkd.NavigationSection(items=[mkd.NavigationItem(label='雀躍', target=compound)]),
        ]))
        dictionary.finish()
        product.finish()
        reopened = mkd.DictionaryProduct.open(path)
        navigation = reopened.navigation
        del reopened
        gc.collect()
        item = navigation.item_at(0, 0)
        self.assertEqual(item.label, '雀躍')
        self.assertEqual(item.target.entry_id, compound)

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
