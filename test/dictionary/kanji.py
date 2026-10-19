import mkd
from common import add_headword, new_dictionary, new_product, write_article


def build(destination):
    product = new_product(destination, mkd.CreationProfile.KANWA)
    dictionary = new_dictionary(product, 'test.birbkanji', 'kanji', encrypted=True)
    targets = []
    for character, reading, compound, compound_reading in (
        ('鳥', 'とり', '鳥語', 'ちょうご'),
        ('羽', 'はね', '羽音', 'はおと'),
    ):
        entry = dictionary.create_entry()
        target = entry.add_target(mkd.EntryTargetKind.COMPOUND)
        write_article(entry, 'kanji.xml', character=character, compound=compound, target=target)
        add_headword(dictionary, entry.target, character, reading)
        dictionary.add_headline(mkd.HeadlineComponents(compound, target, sorting_headline=compound_reading))
        dictionary.add_search_key(mkd.SearchScope.IDIOM, compound_reading, target)
        targets.append(entry.target)
    dictionary.set_entry_order([target.page_id for target in reversed(targets)])
    members = mkd.NavigationIndex(sections=[
        mkd.NavigationSection('部首 ＋ 0画', [mkd.NavigationItem(target=targets[0])]),
    ])
    radicals = mkd.NavigationIndex(kind=mkd.NavigationKind.RADICALS, sections=[
        mkd.NavigationSection('11画', [mkd.NavigationItem(
            label=mkd.RadicalLabel('鳥', reading='とり'), target=members,
        )]),
    ])
    product.set_navigation(mkd.NavigationIndex(sections=[
        mkd.NavigationSection('親字', [mkd.NavigationItem(label='部首', target=radicals)]),
    ]))
    dictionary.finish()
    product.finish()
