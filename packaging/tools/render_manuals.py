#!/usr/bin/env python3
"""Render the English installation and technical manuals with ReportLab.

Run from any directory: python3 packaging/tools/render_manuals.py
Requires reportlab. Uses Arial when installed, otherwise built-in PDF fonts.
"""
from html import escape
from pathlib import Path
import re

from reportlab.lib import colors
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import ParagraphStyle
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.pdfmetrics import Font
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.platypus import BaseDocTemplate, Frame, PageTemplate, Paragraph, Spacer, PageBreak, Table, TableStyle, XPreformatted, HRFlowable
from reportlab.platypus.tableofcontents import TableOfContents

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'output/pdf'
NAVY, TEAL, INK, MUTED, RULE, PALE = [colors.HexColor(c) for c in
    ('#163246', '#007D87', '#243746', '#5F7180', '#D8E1E7', '#F1F5F8')]
PAGE_W, PAGE_H = A4
LEFT, RIGHT, TOP, BOTTOM = 49, 49, 55, 48
WIDTH = PAGE_W - LEFT - RIGHT
FONT_DIR = Path('/System/Library/Fonts/Supplemental')
for name, filename, fallback in [('Body', 'Arial.ttf', 'Helvetica'), ('Body-Bold', 'Arial Bold.ttf', 'Helvetica-Bold'),
                                 ('Body-Italic', 'Arial Italic.ttf', 'Helvetica-Oblique'), ('Mono', 'Courier New.ttf', 'Courier')]:
    if (FONT_DIR / filename).is_file():
        pdfmetrics.registerFont(TTFont(name, str(FONT_DIR / filename)))
    else:
        pdfmetrics.registerFont(Font(name, fallback, 'WinAnsiEncoding'))
pdfmetrics.registerFontFamily('Body', normal='Body', bold='Body-Bold', italic='Body-Italic', boldItalic='Body-Bold')

STYLES = {
    'body': ParagraphStyle('body', fontName='Body', fontSize=10, leading=14.3, textColor=INK, spaceAfter=8),
    'bullet': ParagraphStyle('bullet', fontName='Body', fontSize=9.8, leading=14, textColor=INK,
                             leftIndent=13, firstLineIndent=-11, spaceAfter=7),
    'h2': ParagraphStyle('h2', fontName='Body-Bold', fontSize=22, leading=26.5, textColor=NAVY,
                         spaceAfter=16, keepWithNext=True),
    'h3': ParagraphStyle('h3', fontName='Body-Bold', fontSize=14.5, leading=19, textColor=NAVY,
                         spaceBefore=4, spaceAfter=10, keepWithNext=True),
    'label': ParagraphStyle('label', fontName='Body-Bold', fontSize=8.5, leading=12, textColor=TEAL, spaceAfter=8),
    'small': ParagraphStyle('small', fontName='Body', fontSize=8.7, leading=12.5, textColor=MUTED, spaceAfter=8),
    'cell': ParagraphStyle('cell', fontName='Body', fontSize=9.3, leading=12.8, textColor=INK),
    'cellhead': ParagraphStyle('cellhead', fontName='Body-Bold', fontSize=9.3, leading=13, textColor=colors.white),
    'code': ParagraphStyle('code', fontName='Mono', fontSize=8.2, leading=11.5, textColor=INK, spaceAfter=0),
}


def inline(value):
    tokens = []
    def code(match):
        tokens.append('<font name="Mono" size="8.8">' + escape(match[1]) + '</font>')
        return f'@@TOKEN{len(tokens)-1}@@'
    value = escape(re.sub(r'`([^`]+)`', code, value))
    value = re.sub(r'\[([^\]]+)\]\((https?://[^\s)]+)\)',
                   lambda m: f'<link href="{m[2]}" color="#007D87"><u>{m[1]}</u></link>', value)
    value = re.sub(r'\*\*(.+?)\*\*', r'<b>\1</b>', value)
    for i, token in enumerate(tokens):
        value = value.replace(f'@@TOKEN{i}@@', token)
    return value


def para(text, kind='body'):
    return Paragraph(inline(text), STYLES[kind])


def code_box(value):
    value = value.replace(
        "sudo sh -c 'umask 077; docker exec t-delayer pg_dump --clean --if-exists -U delayer -d delayerdb > /root/delayerdb.sql'",
        '\n'.join(["sudo sh -c 'umask 077", 'docker exec t-delayer pg_dump --clean --if-exists ' + chr(92),
                   "  -U delayer -d delayerdb > /root/delayerdb.sql'"]))
    value = value.replace(
        'sudo env DEBIAN_FRONTEND=noninteractive apt install ./t-delayer-docker_4.1.2-1ubuntu24.04_amd64.deb',
        'sudo env DEBIAN_FRONTEND=noninteractive apt install ' + chr(92) + '\n  ./t-delayer-docker_4.1.2-1ubuntu24.04_amd64.deb')
    assert all(pdfmetrics.stringWidth(line, 'Mono', 8.2) <= WIDTH-24 for line in value.splitlines()), value
    box = Table([[XPreformatted(escape(value), STYLES['code'])]], colWidths=[WIDTH], hAlign='LEFT')
    box.setStyle(TableStyle([('BACKGROUND', (0, 0), (-1, -1), PALE), ('BOX', (0, 0), (-1, -1), .5, RULE),
        ('LEFTPADDING', (0, 0), (-1, -1), 12), ('RIGHTPADDING', (0, 0), (-1, -1), 12),
        ('TOPPADDING', (0, 0), (-1, -1), 9), ('BOTTOMPADDING', (0, 0), (-1, -1), 9)]))
    box.spaceBefore, box.spaceAfter = 2, 11
    return box


def table(lines):
    rows = [[x.strip() for x in line.strip().strip('|').split('|')] for line in lines]
    rows = [rows[0]] + rows[2:]
    first = 235 if rows[0][0] == 'Host path' else 145
    box = Table([[para(cell, 'cellhead' if i == 0 else 'cell') for cell in row] for i, row in enumerate(rows)],
                colWidths=[first, WIDTH-first], repeatRows=1, hAlign='LEFT')
    styling = [('BACKGROUND', (0, 0), (-1, 0), NAVY), ('VALIGN', (0, 0), (-1, -1), 'TOP'),
        ('LEFTPADDING', (0, 0), (-1, -1), 9), ('RIGHTPADDING', (0, 0), (-1, -1), 9),
        ('TOPPADDING', (0, 0), (-1, -1), 8), ('BOTTOMPADDING', (0, 0), (-1, -1), 8),
        ('LINEBELOW', (0, 1), (-1, -1), .45, RULE)]
    styling.extend(('BACKGROUND', (0, i), (-1, i), PALE) for i in range(2, len(rows), 2))
    box.setStyle(TableStyle(styling))
    box.spaceAfter = 13
    return box


class ManualDoc(BaseDocTemplate):
    def __init__(self, filename, subtitle):
        self.subtitle = subtitle
        super().__init__(str(filename), pagesize=A4, leftMargin=LEFT, rightMargin=RIGHT, topMargin=TOP,
                         bottomMargin=BOTTOM, title='T-Delayer: ' + subtitle, author='T-Delayer',
                         subject='Docker package 4.1.2 for Ubuntu 24.04 and 26.04 amd64')
        frame = Frame(LEFT, BOTTOM, WIDTH, PAGE_H-TOP-BOTTOM, leftPadding=0, rightPadding=0, topPadding=0, bottomPadding=0)
        self.addPageTemplates(PageTemplate(id='manual', frames=[frame], onPage=self.decorate))

    def decorate(self, canvas, doc):
        canvas.saveState()
        if doc.page > 1 or self.subtitle == 'Technical manual':
            canvas.setFont('Body-Bold', 8); canvas.setFillColor(NAVY)
            canvas.drawString(LEFT, PAGE_H-29, 'T-DELAYER')
            canvas.setFont('Body', 8); canvas.setFillColor(MUTED)
            canvas.drawRightString(PAGE_W-RIGHT, PAGE_H-29, self.subtitle.upper())
            canvas.setStrokeColor(RULE); canvas.line(LEFT, PAGE_H-38, PAGE_W-RIGHT, PAGE_H-38)
        canvas.setStrokeColor(RULE); canvas.line(LEFT, 34, PAGE_W-RIGHT, 34)
        canvas.setFillColor(MUTED); canvas.setFont('Body', 8)
        canvas.drawString(LEFT, 21, 'Docker package 4.1.2  |  Ubuntu 24.04 / 26.04  |  amd64')
        canvas.drawRightString(PAGE_W-RIGHT, 21, str(doc.page))
        canvas.restoreState()

    def afterFlowable(self, item):
        if hasattr(item, 'bookmark'):
            self.canv.bookmarkPage(item.bookmark)
            self.canv.addOutlineEntry(item.title, item.bookmark, 0)
            self.notify('TOCEntry', (0, item.title, self.page, item.bookmark))


def render(source_file, output_file, technical=False):
    source = source_file.read_text()
    story = []
    first_section = source.index('\n## ')
    intro = source[:first_section].split('\n', 2)[2].strip()
    if technical:
        story += [para('TECHNICAL MANUAL / DOCKER PACKAGE 4.1.2', 'label'), para('T-Delayer', 'h2'), para(intro), Spacer(1, 8)]
    else:
        story += [Spacer(1, 28), para('DOCKER PACKAGE 4.1.2', 'label')]
        title = ParagraphStyle('cover', fontName='Body-Bold', fontSize=40, leading=44, textColor=NAVY, spaceAfter=10)
        sub = ParagraphStyle('cover-subtitle', fontName='Body', fontSize=23, leading=29, textColor=NAVY, spaceAfter=15)
        story += [Paragraph('T-Delayer', title), Paragraph('Installation and<br/>administration', sub),
                  para('Ubuntu Server 24.04 LTS and 26.04 LTS  |  amd64', 'small'),
                  HRFlowable(width=WIDTH, thickness=2, color=TEAL, spaceBefore=7, spaceAfter=19),
                  para(' '.join(intro.splitlines())), Spacer(1, 24), para('CONTENTS', 'label')]
        toc = TableOfContents()
        toc.levelStyles = [ParagraphStyle('toc', fontName='Body', fontSize=10.5, leading=23, textColor=INK)]
        toc.dotsMinLevel = 0
        story.append(toc)
    lines, chapter, i = source[first_section:].strip().splitlines(), 0, 0
    while i < len(lines):
        line = lines[i]
        if not line.strip():
            i += 1; continue
        if line.startswith('## '):
            chapter += 1
            if not technical:
                story += [PageBreak(), para(f'{chapter:02d} / ADMINISTRATION GUIDE', 'label')]
            elif chapter > 1:
                story += [PageBreak(), para(f'{chapter:02d} / TECHNICAL GUIDE', 'label')]
            heading = para(line[3:], 'h2')
            heading.bookmark, heading.title = f'chapter-{chapter}', line[3:]
            story.append(heading); i += 1; continue
        if line.startswith('```'):
            code = []; i += 1
            while i < len(lines) and not lines[i].startswith('```'):
                code.append(lines[i]); i += 1
            story.append(code_box('\n'.join(code))); i += 1; continue
        if line.startswith('|'):
            rows = []
            while i < len(lines) and lines[i].startswith('|'):
                rows.append(lines[i]); i += 1
            story.append(table(rows)); continue
        bullet, values = line.startswith('- '), [line[2:] if line.startswith('- ') else line]
        i += 1
        while i < len(lines) and lines[i].strip() and not lines[i].startswith(('## ', '```', '|', '- ')):
            values.append(lines[i].strip()); i += 1
        text = ' '.join(values)
        if text.startswith('**An Ubuntu 24.04 to 26.04'):
            story += [PageBreak(), para(f'{chapter:02d} / OPERATIONS, BACKUP, UPGRADES, AND REMOVAL', 'label'),
                      para('Ubuntu release migration', 'h3')]
        story.append(para(('\u2022 ' if bullet else '') + text, 'bullet' if bullet else 'body'))
    ManualDoc(output_file, 'Technical manual' if technical else 'Installation and administration').multiBuild(story)
    print(output_file)


if __name__ == '__main__':
    OUT.mkdir(parents=True, exist_ok=True)
    render(ROOT/'packaging/INSTALL.md', OUT/'T-Delayer_Installation_Manual.pdf')
    render(ROOT/'packaging/TECHNICAL.md', OUT/'T-Delayer_Technical_Manual.pdf', technical=True)
