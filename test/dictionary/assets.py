import io
import wave

import mkd
from common import ASSETS


def chirp():
    output = io.BytesIO()
    with wave.open(output, 'wb') as audio:
        audio.setparams((1, 2, 8000, 0, 'NONE', 'not compressed'))
        audio.writeframes(b'\0\0' * 80)
    return output.getvalue()


def add_resources(dictionary):
    dictionary.add_graphic('saberlion.png', (ASSETS / 'saberlion.png').read_bytes())
    dictionary.add_audio('chirp.wav', chirp())
    dictionary.add_font('BirbMincho.ttf', (ASSETS / 'BirbMincho.ttf').read_bytes())
    dictionary.add_asset('birbs.css', b'body { font-family: BirbMincho; }')
    dictionary.add_asset('birbs-nightmode.css', b'body { color: white; }')
    dictionary.add_asset('birbs-ruby.css', b'rt { display: inline; }')
    dictionary.set_presentation(mkd.DictionaryPresentation(full_ruby_stylesheet='birbs-ruby.css'))
    return dictionary.add_gaiji('tsugumi.svg', (ASSETS / 'gaiji.svg').read_bytes())


def add_information(product):
    product.set_icon((ASSETS / 'icon.png').read_bytes())
    product.set_information(mkd.ProductInformation(
        copyright=mkd.LocalizedString(en='Original birb descriptions for mkd tests.'),
        licenses=[mkd.ProductLicense('Birb Mincho', (ASSETS / 'font-OFL.txt').read_text(encoding='utf-8'))],
    ))
