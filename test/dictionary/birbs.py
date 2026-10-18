import mkd
from assets import add_information, add_resources
from common import add_headword, new_dictionary, new_product, write_article


def build(destination):
    product = new_product(destination)
    dictionary = new_dictionary(product)
    add_information(product)
    gaiji = add_resources(dictionary)

    tsugumi = dictionary.create_entry()
    suzume = dictionary.create_entry()
    saberlion = dictionary.create_entry()
    idiom = tsugumi.add_target(mkd.EntryTargetKind.IDIOM)
    example = tsugumi.add_target(mkd.EntryTargetKind.EXAMPLE)
    sense = saberlion.add_target(mkd.EntryTargetKind.SENSE)

    label = mkd.InlineText()
    label.append(mkd.Ruby('鶫', 'つぐみ'))
    label.append(mkd.Gaiji(gaiji, '鶫'))
    write_article(tsugumi, 'tsugumi.xml', title=label.to_xml(), idiom=idiom,
                  example=example, saberlion=saberlion.target)
    write_article(suzume, 'suzume.xml')
    write_article(saberlion, 'saberlion.xml', sense=sense, tsugumi=tsugumi.target)

    add_headword(dictionary, tsugumi.target, '鶫', 'つぐみ', 'birb', 'spotted birb')
    add_headword(dictionary, suzume.target, '雀', 'すずめ', 'birb', 'small birb')
    add_headword(dictionary, saberlion.target, 'セイバーライオン', 'がおー', 'saberlion')
    for target, title, key, scope in (
        (idiom, '一石二鳥', 'いっせきにちょう', mkd.SearchScope.IDIOM),
        (example, '鶫がさえずる', 'さえずる', mkd.SearchScope.EXAMPLE),
        (sense, '食いしん坊の獅子', 'くいしんぼう', mkd.SearchScope.SENSE),
    ):
        dictionary.add_headline(mkd.HeadlineComponents(title, target))
        dictionary.add_search_key(scope, key, target, exact_only=scope == mkd.SearchScope.IDIOM)
    dictionary.add_search_key(mkd.SearchScope.ENGLISH, 'thrush', tsugumi.target)
    dictionary.add_search_key(mkd.SearchScope.ENGLISH, 'sparrow', suzume.target)
    dictionary.add_headline(mkd.HeadlineComponents('つぐみ', tsugumi.target), role=mkd.HeadlineRole.SECONDARY)
    dictionary.add_headline(mkd.HeadlineComponents('鶫', tsugumi.target), role=mkd.HeadlineRole.SHORT)
    dictionary.set_entry_order([saberlion.target.page_id, tsugumi.target.page_id, suzume.target.page_id])
    dictionary.add_appendix_entries('friends', '小鳥と獅子', [
        mkd.AppendixEntry(tsugumi.target, label='鶫'),
        mkd.AppendixEntry(saberlion.target, label='セイバーライオン'),
    ])
    dictionary.add_asset('guide.html', b'<html><body>A small flock, plus one hungry lion.</body></html>')
    dictionary.add_appendix('guide.html', 'この辞典の見方')
    dictionary.finish()
    product.finish()
