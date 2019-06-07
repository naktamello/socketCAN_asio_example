import re
from dataclasses import dataclass

regex = re.compile("(-?\d+)\s'([^']+)'\s?")


@dataclass
class Character:
    num: int
    char: str


def parse_char_array(lines):
    array = []
    for line in lines:
        text = line.split(":")[-1].strip()
        match = regex.findall(text)
        array += [Character(int(n),c) for (n,c) in match]
    return array

if __name__ == '__main__':
    with open("sample_message.txt") as infile:
        lines = infile.readlines()
    array = parse_char_array(lines)
    payload = [c.num for c in array]
    print(payload)
