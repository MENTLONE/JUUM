from pathlib import Path
from bs4 import BeautifulSoup
from reportlab.lib import colors
from reportlab.lib.enums import TA_CENTER
from reportlab.lib.pagesizes import letter
from reportlab.lib.styles import getSampleStyleSheet, ParagraphStyle
from reportlab.lib.units import inch
from reportlab.platypus import SimpleDocTemplate, Paragraph, Spacer, ListFlowable, ListItem

ROOT = Path(__file__).resolve().parents[1]
html = (ROOT / "manual.html").read_text(encoding="utf-8")
soup = BeautifulSoup(html, "html.parser")
story = []
styles = getSampleStyleSheet()
styles.add(ParagraphStyle(name="ManualTitle", parent=styles["Title"], fontSize=30, leading=34, alignment=TA_CENTER, textColor=colors.HexColor("#087da5"), spaceAfter=8))
styles.add(ParagraphStyle(name="ManualSub", parent=styles["Normal"], fontSize=11, leading=15, alignment=TA_CENTER, textColor=colors.HexColor("#166d85"), spaceAfter=18))
styles.add(ParagraphStyle(name="ManualH2", parent=styles["Heading1"], fontSize=16, leading=20, textColor=colors.HexColor("#087da5"), spaceBefore=12, spaceAfter=7))
styles.add(ParagraphStyle(name="ManualH3", parent=styles["Heading2"], fontSize=11, leading=14, textColor=colors.HexColor("#155e75"), spaceBefore=7, spaceAfter=3))
styles.add(ParagraphStyle(name="ManualBody", parent=styles["BodyText"], fontSize=9.5, leading=13, spaceAfter=5))
styles.add(ParagraphStyle(name="ManualSmall", parent=styles["BodyText"], fontSize=8, leading=10, textColor=colors.HexColor("#526b75"), spaceAfter=5))

def safe_markup(node):
    # Preserve basic inline emphasis while escaping text for ReportLab's XML parser.
    from xml.sax.saxutils import escape
    if isinstance(node, str):
        return escape(node)
    if getattr(node, "name", None) is None:
        return escape(str(node))
    tag = node.name.lower()
    content = "".join(safe_markup(c) for c in node.children)
    if tag in ("strong", "b"):
        return "<b>" + content + "</b>"
    if tag in ("em", "i"):
        return "<i>" + content + "</i>"
    if tag == "br":
        return "<br/>"
    if tag == "a":
        href = node.get("href", "")
        if href.startswith("http"):
            return content + " (" + escape(href) + ")"
        return content
    return content

main = soup.find("main") or soup.body
for section in main.find_all(["section", "footer"], recursive=False):
    for el in section.find_all(["h1", "h2", "h3", "p", "ol", "ul"], recursive=False):
        if el.name == "h1":
            story.append(Paragraph(safe_markup(el), styles["ManualTitle"]))
        elif el.name == "h2":
            story.append(Paragraph(safe_markup(el), styles["ManualH2"]))
        elif el.name == "h3":
            story.append(Paragraph(safe_markup(el), styles["ManualH3"]))
        elif el.name == "p":
            story.append(Paragraph(safe_markup(el), styles["ManualBody"]))
        elif el.name in ("ol", "ul"):
            items = []
            for li in el.find_all("li", recursive=False):
                items.append(ListItem(Paragraph(safe_markup(li), styles["ManualBody"]), leftIndent=10))
            story.append(ListFlowable(items, bulletType="1" if el.name == "ol" else "bullet", leftIndent=18, bulletFontSize=7, spaceAfter=6))
    story.append(Spacer(1, 5))

def footer(canvas, doc):
    canvas.saveState()
    canvas.setFont("Helvetica", 8)
    canvas.setFillColor(colors.HexColor("#526b75"))
    canvas.drawString(0.65*inch, 0.35*inch, "JUUM User Manual")
    canvas.drawRightString(letter[0]-0.65*inch, 0.35*inch, f"Page {doc.page}")
    canvas.restoreState()

doc = SimpleDocTemplate(str(ROOT / "JUUM_User_Manual.pdf"), pagesize=letter, rightMargin=.65*inch, leftMargin=.65*inch, topMargin=.6*inch, bottomMargin=.6*inch, title="JUUM User Manual")
doc.build(story, onFirstPage=footer, onLaterPages=footer)
print("Built JUUM_User_Manual.pdf from manual.html")
