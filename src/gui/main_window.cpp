#include "main_window.hpp"

#include "app/cli_commands.hpp"
#include "cli_output.hpp"
#include "core/unit_conversion.hpp"
#include "gui/matrix_editor.hpp"

#include <QComboBox>
#include <QCheckBox>
#include <QClipboard>
#include <QApplication>
#include <QDesktopServices>
#include <QDir>
#include <QDateTime>
#include <QFontDatabase>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QRandomGenerator>
#include <QFrame>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTabWidget>
#include <QToolButton>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QSpinBox>
#include <QPixmap>
#include <QPalette>
#include <QPainter>
#include <QSplitter>
#include <QUrl>
#include <QTextBrowser>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QSignalBlocker>
#include <QProcess>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QTextDocument>
#include <QKeyEvent>
#include <QEvent>
#include <QTextEdit>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>

namespace {
bool g_isDarkTheme = false;

struct PythonRunResult {
  bool started = false;
  int exitCode = -1;
  QString stdoutText;
  QString stderrText;
  QString errorMessage;
};

struct CliRunResult {
  int exitCode = 0;
  std::string out;
  std::string err;
};

class StreamRedirect {
public:
  StreamRedirect(std::ostream &stream, std::streambuf *newBuffer)
      : stream_(stream), oldBuffer_(stream.rdbuf(newBuffer)) {}

  ~StreamRedirect() { stream_.rdbuf(oldBuffer_); }

private:
  std::ostream &stream_;
  std::streambuf *oldBuffer_;
};

CliRunResult runCliAction(const std::function<int()> &action) {
  std::ostringstream outBuffer;
  std::ostringstream errBuffer;
  StreamRedirect outRedirect(std::cout, outBuffer.rdbuf());
  StreamRedirect errRedirect(std::cerr, errBuffer.rdbuf());

  CliRunResult result;
  result.exitCode = action();
  result.out = outBuffer.str();
  result.err = errBuffer.str();
  return result;
}

std::vector<std::string> splitTokens(const QString &text) {
  std::istringstream stream(text.toStdString());
  std::vector<std::string> tokens;
  std::string token;
  while (stream >> token) {
    tokens.push_back(token);
  }
  return tokens;
}

QGroupBox *wrapSection(const QString &title, QLayout *layout) {
  auto *group = new QGroupBox(title);
  group->setProperty("sectionCard", true);
  group->setLayout(layout);
  return group;
}

std::string toLower(std::string value) {
  for (char &ch : value) {
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  }
  return value;
}

QString renderMarkdownWithLatexSymbols(const QString &source) {
  QString text = source;
  static const std::vector<std::pair<QString, QString>> replacements = {
      {"\\\\alpha", "α"}, {"\\\\beta", "β"},   {"\\\\gamma", "γ"},
      {"\\\\delta", "δ"}, {"\\\\epsilon", "ε"}, {"\\\\theta", "θ"},
      {"\\\\lambda", "λ"}, {"\\\\mu", "μ"},    {"\\\\pi", "π"},
      {"\\\\sigma", "σ"}, {"\\\\phi", "φ"},    {"\\\\omega", "ω"},
      {"\\\\Delta", "Δ"}, {"\\\\Sigma", "Σ"},  {"\\\\Pi", "Π"},
      {"\\\\Omega", "Ω"}, {"\\\\times", "×"},  {"\\\\cdot", "·"},
      {"\\\\le", "≤"},    {"\\\\ge", "≥"},     {"\\\\neq", "≠"},
      {"\\\\approx", "≈"}, {"\\\\pm", "±"},    {"\\\\infty", "∞"},
      {"\\\\rightarrow", "→"}, {"\\\\leftarrow", "←"},
      {"\\\\leftrightarrow", "↔"}};

  for (const auto &item : replacements) {
    text.replace(item.first, item.second);
  }
  text.replace("$$", "");
  text.replace("$", "");
  return text;
}

QString escapeWithWhitespace(const QString &text) {
  QString out;
  out.reserve(text.size() * 2);
  for (const QChar ch : text) {
    if (ch == '&') {
      out += "&amp;";
    } else if (ch == '<') {
      out += "&lt;";
    } else if (ch == '>') {
      out += "&gt;";
    } else if (ch == '"') {
      out += "&quot;";
    } else if (ch == '\'') {
      out += "&#39;";
    } else if (ch == '\t') {
      out += "&nbsp;&nbsp;&nbsp;&nbsp;";
    } else if (ch == '\n') {
      out += "<br/>";
    } else if (ch == ' ') {
      out += "&nbsp;";
    } else {
      out += ch;
    }
  }
  return out;
}

QString wrapCodeBlockHtml(int index, const QString &code, const QString &language,
                          const QString &output) {
  const bool useDark = g_isDarkTheme;
  const QString label =
      language.trimmed().isEmpty() ? QStringLiteral("code") : language.trimmed();
  const QString lang = label.toLower();
  const bool isPython = (lang == "py" || lang == "python");
  const QString background = useDark ? "#0b0f14" : "#f7f7f9";
  const QString border = useDark ? "#1f2a3a" : "#c9c9c9";
  const QString header = useDark ? "#101622" : "#f0f1f5";
  const QString text = useDark ? "#e6edf7" : "#1b1f24";
  const QString subtle = useDark ? "#93a4bb" : "#667085";
  const QString link = useDark ? "#6ab0ff" : "#2c7be5";

  struct HighlightRule {
    QRegularExpression regex;
    QString darkStyle;
    QString lightStyle;
  };

  auto applyHighlighting = [&](const QString &source,
                               const std::vector<HighlightRule> &rules) {
    struct Token {
      int start = 0;
      int end = 0;
      QString style;
    };
    std::vector<Token> tokens;
    for (const auto &rule : rules) {
      QRegularExpressionMatchIterator it = rule.regex.globalMatch(source);
      while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        const int start = match.capturedStart();
        const int end = match.capturedEnd();
        if (start < 0 || end <= start) {
          continue;
        }
        bool overlaps = false;
        for (const auto &token : tokens) {
          if (start < token.end && end > token.start) {
            overlaps = true;
            break;
          }
        }
        if (overlaps) {
          continue;
        }
        tokens.push_back(
            {start, end, useDark ? rule.darkStyle : rule.lightStyle});
      }
    }
    std::sort(tokens.begin(), tokens.end(),
              [](const Token &a, const Token &b) { return a.start < b.start; });
    QString result;
    int pos = 0;
    for (const auto &token : tokens) {
      if (token.start > pos) {
        result += escapeWithWhitespace(source.mid(pos, token.start - pos));
      }
      const QString segment = source.mid(token.start, token.end - token.start);
      result += QString("<span style=\"%1\">%2</span>")
                    .arg(token.style, escapeWithWhitespace(segment));
      pos = token.end;
    }
    if (pos < source.size()) {
      result += escapeWithWhitespace(source.mid(pos));
    }
    return result;
  };

  std::vector<HighlightRule> rules;
  const QString commentDark = "color:#565f89;";
  const QString commentLight = "color:#6e7781;";
  const QString stringDark = "color:#9ece6a;";
  const QString stringLight = "color:#0a7a3d;";
  const QString numberDark = "color:#ff9e64;";
  const QString numberLight = "color:#b36200;";
  const QString keywordDark = "color:#7aa2f7; font-weight:600;";
  const QString keywordLight = "color:#1f6feb; font-weight:600;";
  const QString typeDark = "color:#7dcfff;";
  const QString typeLight = "color:#0550ae;";
  const QString builtinDark = "color:#c0caf5;";
  const QString builtinLight = "color:#24292f;";

  auto addRule = [&](const QString &pattern, QRegularExpression::PatternOptions options,
                     const QString &dark, const QString &light) {
    rules.push_back({QRegularExpression(pattern, options), dark, light});
  };

  if (lang.contains("python") || lang == "py") {
    addRule("'''[\\s\\S]*?'''", QRegularExpression::DotMatchesEverythingOption,
            stringDark, stringLight);
    addRule("\"\"\"[\\s\\S]*?\"\"\"", QRegularExpression::DotMatchesEverythingOption,
            stringDark, stringLight);
    addRule("(?<!\\\\)\"([^\"\\\\]|\\\\.)*\"", QRegularExpression::NoPatternOption,
            stringDark, stringLight);
    addRule("(?<!\\\\)'([^'\\\\]|\\\\.)*'", QRegularExpression::NoPatternOption,
            stringDark, stringLight);
    addRule("#[^\\n]*", QRegularExpression::NoPatternOption, commentDark,
            commentLight);
    addRule("\\b\\d+(?:\\.\\d+)?\\b", QRegularExpression::NoPatternOption,
            numberDark, numberLight);
    addRule("\\b(def|class|return|if|elif|else|for|while|break|continue|import|from|"
            "as|pass|raise|try|except|with|lambda|True|False|None)\\b",
            QRegularExpression::NoPatternOption, keywordDark, keywordLight);
    addRule("\\b(print|len|range|dict|list|set|tuple|str|int|float|bool)\\b",
            QRegularExpression::NoPatternOption, builtinDark, builtinLight);
  } else if (lang.contains("json")) {
    addRule("\"([^\"\\\\]|\\\\.)*\"", QRegularExpression::NoPatternOption,
            stringDark, stringLight);
    addRule("\\b(true|false|null)\\b", QRegularExpression::NoPatternOption,
            keywordDark, keywordLight);
    addRule("\\b-?\\d+(?:\\.\\d+)?\\b", QRegularExpression::NoPatternOption,
            numberDark, numberLight);
  } else if (lang.contains("yaml") || lang.contains("yml")) {
    addRule("^\\s*[^:\\n]+(?=\\s*:)", QRegularExpression::MultilineOption,
            typeDark, typeLight);
    addRule("\"([^\"\\\\]|\\\\.)*\"", QRegularExpression::NoPatternOption,
            stringDark, stringLight);
    addRule("'([^'\\\\]|\\\\.)*'", QRegularExpression::NoPatternOption,
            stringDark, stringLight);
    addRule("#[^\\n]*", QRegularExpression::NoPatternOption, commentDark,
            commentLight);
    addRule("\\b(true|false|null)\\b", QRegularExpression::NoPatternOption,
            keywordDark, keywordLight);
    addRule("\\b-?\\d+(?:\\.\\d+)?\\b", QRegularExpression::NoPatternOption,
            numberDark, numberLight);
  } else if (lang.contains("bash") || lang.contains("sh")) {
    addRule("(?<!\\\\)\"([^\"\\\\]|\\\\.)*\"", QRegularExpression::NoPatternOption,
            stringDark, stringLight);
    addRule("(?<!\\\\)'([^'\\\\]|\\\\.)*'", QRegularExpression::NoPatternOption,
            stringDark, stringLight);
    addRule("#[^\\n]*", QRegularExpression::NoPatternOption, commentDark,
            commentLight);
    addRule("\\b(echo|export|cd|ls|cat|grep|sed|awk|curl|sudo|if|then|fi|for|do|done)\\b",
            QRegularExpression::NoPatternOption, keywordDark, keywordLight);
    addRule("\\b-?\\d+(?:\\.\\d+)?\\b", QRegularExpression::NoPatternOption,
            numberDark, numberLight);
  } else if (lang.contains("js") || lang.contains("ts") || lang.contains("javascript") ||
             lang.contains("typescript")) {
    addRule("`([^`\\\\]|\\\\.)*`", QRegularExpression::NoPatternOption,
            stringDark, stringLight);
    addRule("(?<!\\\\)\"([^\"\\\\]|\\\\.)*\"", QRegularExpression::NoPatternOption,
            stringDark, stringLight);
    addRule("(?<!\\\\)'([^'\\\\]|\\\\.)*'", QRegularExpression::NoPatternOption,
            stringDark, stringLight);
    addRule("/\\*[\\s\\S]*?\\*/", QRegularExpression::DotMatchesEverythingOption,
            commentDark, commentLight);
    addRule("//[^\\n]*", QRegularExpression::NoPatternOption, commentDark,
            commentLight);
    addRule("\\b(const|let|var|function|return|if|else|for|while|break|continue|"
            "class|new|try|catch|throw|import|from|export|default|async|await|"
            "true|false|null|undefined)\\b",
            QRegularExpression::NoPatternOption, keywordDark, keywordLight);
    addRule("\\b-?\\d+(?:\\.\\d+)?\\b", QRegularExpression::NoPatternOption,
            numberDark, numberLight);
  } else if (lang.contains("cpp") || lang.contains("c++") || lang == "c" ||
             lang == "h" || lang.contains("hpp") || lang.contains("hxx") ||
             lang.contains("cc") || lang.contains("cxx")) {
    addRule("(?<!\\\\)\"([^\"\\\\]|\\\\.)*\"", QRegularExpression::NoPatternOption,
            stringDark, stringLight);
    addRule("(?<!\\\\)'([^'\\\\]|\\\\.)*'", QRegularExpression::NoPatternOption,
            stringDark, stringLight);
    addRule("/\\*[\\s\\S]*?\\*/", QRegularExpression::DotMatchesEverythingOption,
            commentDark, commentLight);
    addRule("//[^\\n]*", QRegularExpression::NoPatternOption, commentDark,
            commentLight);
    addRule("\\b(int|float|double|auto|void|const|constexpr|struct|class|public|"
            "private|protected|virtual|override|template|typename|using|namespace|"
            "static|inline|return|if|else|switch|case|for|while|do|break|continue|"
            "new|delete|try|catch|throw|include)\\b",
            QRegularExpression::NoPatternOption, keywordDark, keywordLight);
    addRule("\\b(bool|char|short|long|size_t|string|vector|map|unordered_map)\\b",
            QRegularExpression::NoPatternOption, typeDark, typeLight);
    addRule("\\b-?\\d+(?:\\.\\d+)?\\b", QRegularExpression::NoPatternOption,
            numberDark, numberLight);
  }

  const QString rendered = rules.empty() ? escapeWithWhitespace(code)
                                         : applyHighlighting(code, rules);
  const QString outputHtml = output.trimmed().isEmpty()
                                 ? QString()
                                 : QString(
                                       "<div style=\"border-top:1px solid %1; "
                                       "background:%2;\">"
                                       "<div style=\"padding:6px 10px; "
                                       "font-size:12px; color:%3;\">OUTPUT</div>"
                                       "<pre style=\"margin:0; padding:10px; "
                                       "font-family:'JetBrains Mono','Fira Code',"
                                       "'Source Code Pro','IBM Plex Mono','Menlo',"
                                       "'Consolas',monospace; font-size:12px; "
                                       "line-height:1.5;\">%4</pre>"
                                       "</div>")
                                       .arg(border, background, subtle,
                                            escapeWithWhitespace(output));
  const QString runLink =
      isPython ? QString("<a style=\"color:%1; text-decoration:none; margin-right:18px;\" "
                         "href=\"run://%2\">Run Cell</a>")
                      .arg(link)
                      .arg(index)
               : QString();
  return QString(
             "<div style=\"border:1px solid %1; border-radius:8px; "
             "margin:10px 0; background:%2; color:%3;\">"
             "<div style=\"padding:6px 10px; background:%4; "
             "border-bottom:1px solid %1; border-top-left-radius:8px; "
             "border-top-right-radius:8px; font-size:12px; color:%5;\">"
             "<table width=\"100%%\" cellspacing=\"0\" cellpadding=\"0\">"
             "<tr><td style=\"letter-spacing:0.6px; text-transform:uppercase;\">%6</td>"
             "<td align=\"right\">"
             "%7"
             "<a style=\"color:%8; text-decoration:none;\" href=\"copy://%9\">Copy</a>"
             "</td></tr></table>"
             "</div>"
             "<pre style=\"margin:0; padding:10px; "
             "font-family:'JetBrains Mono','Fira Code','Source Code Pro',"
             "'IBM Plex Mono','Menlo','Consolas',monospace; font-size:12px; "
             "line-height:1.5;\">%10</pre>"
             "%11"
             "</div>")
      .arg(border)
      .arg(background)
      .arg(text)
      .arg(header)
      .arg(subtle)
      .arg(label.toHtmlEscaped())
      .arg(runLink)
      .arg(link)
      .arg(index)
      .arg(rendered)
      .arg(outputHtml);
}

std::vector<QString> extractPythonBlocks(const QString &markdown) {
  std::vector<QString> blocks;
  const QRegularExpression fenceRe(
      "```\\s*(python|py)\\b[^\\n]*\\n([\\s\\S]*?)```",
      QRegularExpression::CaseInsensitiveOption);
  QRegularExpressionMatchIterator it = fenceRe.globalMatch(markdown);
  while (it.hasNext()) {
    QRegularExpressionMatch match = it.next();
    const QString code = match.captured(2);
    if (!code.trimmed().isEmpty()) {
      blocks.push_back(code);
    }
  }
  return blocks;
}

bool isPythonNotesFile(const QString &path) {
  return path.endsWith(".py", Qt::CaseInsensitive);
}

std::vector<std::pair<int, int>> extractCodeRanges(const QString &markdown) {
  std::vector<std::pair<int, int>> ranges;
  const QRegularExpression fenceRe("```[^\\n]*\\n([\\s\\S]*?)```");
  QRegularExpressionMatchIterator it = fenceRe.globalMatch(markdown);
  while (it.hasNext()) {
    QRegularExpressionMatch match = it.next();
    const int start = match.capturedStart(1);
    const int end = match.capturedEnd(1);
    if (start >= 0 && end >= start) {
      ranges.emplace_back(start, end);
    }
  }
  return ranges;
}

std::vector<std::pair<int, int>> extractPythonRanges(const QString &markdown) {
  std::vector<std::pair<int, int>> ranges;
  const QRegularExpression fenceRe(
      "```\\s*(python|py)\\b[^\\n]*\\n([\\s\\S]*?)```",
      QRegularExpression::CaseInsensitiveOption);
  QRegularExpressionMatchIterator it = fenceRe.globalMatch(markdown);
  while (it.hasNext()) {
    QRegularExpressionMatch match = it.next();
    const int start = match.capturedStart(2);
    const int end = match.capturedEnd(2);
    if (start >= 0 && end >= start) {
      ranges.emplace_back(start, end);
    }
  }
  return ranges;
}

bool parseCodeBlockIndex(const QUrl &url, int *index) {
  if (!index) {
    return false;
  }
  const QString display = url.toDisplayString(QUrl::FullyDecoded);
  const QRegularExpression anyDigits("(\\d+)");
  const QRegularExpressionMatch anyMatch = anyDigits.match(display);
  if (anyMatch.hasMatch()) {
    bool ok = false;
    const int value = anyMatch.captured(1).toInt(&ok);
    if (ok) {
      *index = value;
      return true;
    }
  }
  QString raw = url.path();
  raw = raw.trimmed();
  while (raw.startsWith('/')) {
    raw = raw.mid(1);
  }
  while (raw.endsWith('/')) {
    raw.chop(1);
  }
  if (raw.isEmpty()) {
    raw = url.host().trimmed();
  }
  if (raw.isEmpty()) {
    const QString full = url.toString(QUrl::FullyDecoded);
    const QRegularExpression re("^(?:copy|run):/*(\\d+)");
    const QRegularExpressionMatch match = re.match(full);
    if (match.hasMatch()) {
      raw = match.captured(1);
    } else {
      const QRegularExpression tailRe("(\\d+)$");
      const QRegularExpressionMatch tailMatch = tailRe.match(full);
      if (tailMatch.hasMatch()) {
        raw = tailMatch.captured(1);
      }
    }
  }
  bool ok = false;
  const int value = raw.toInt(&ok);
  if (!ok) {
    return false;
  }
  *index = value;
  return true;
}

PythonRunResult runPythonSnippet(const QString &code) {
  PythonRunResult result;
  if (code.trimmed().isEmpty()) {
    result.errorMessage = "No code to run.";
    return result;
  }

  auto runPython = [&code](const QString &program, PythonRunResult *out) -> bool {
    QProcess process;
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    const QString snapRoot = QString::fromUtf8(qgetenv("SNAP"));
    if (!snapRoot.isEmpty()) {
      env.insert("PYTHONHOME", snapRoot + "/usr");
      env.insert("PYTHONPATH", snapRoot + "/usr/lib/python3/dist-packages");
    }
    process.setProcessEnvironment(env);
    process.start(program, QStringList() << "-");
    if (!process.waitForStarted(2000)) {
      return false;
    }
    process.write(code.toUtf8());
    process.closeWriteChannel();
    if (!process.waitForFinished(-1)) {
      return false;
    }
    out->stdoutText = QString::fromUtf8(process.readAllStandardOutput());
    out->stderrText = QString::fromUtf8(process.readAllStandardError());
    out->exitCode = process.exitCode();
    return true;
  };

  const QString snapRoot = QString::fromUtf8(qgetenv("SNAP"));
  QStringList candidates;
  if (!snapRoot.isEmpty()) {
    candidates << (snapRoot + "/usr/bin/python3")
               << (snapRoot + "/usr/bin/python");
  }
  candidates << "python3" << "python";
  for (const auto &candidate : candidates) {
    if (runPython(candidate, &result)) {
      result.started = true;
      break;
    }
  }
  if (!result.started) {
    result.errorMessage =
        "Failed to start Python interpreter (python3/python not found).";
  }
  return result;
}

QFont chooseFont(const QStringList &candidates, int pointSize,
                 QFont::Weight weight = QFont::Normal) {
  QFontDatabase database;
  for (const auto &family : candidates) {
    if (database.families().contains(family)) {
      QFont font(family);
      font.setPointSize(pointSize);
      font.setWeight(weight);
      return font;
    }
  }
  QFont fallback = QApplication::font();
  fallback.setPointSize(pointSize);
  fallback.setWeight(weight);
  return fallback;
}

QFont devMonoFont() {
  const QStringList monoCandidates = {
      QStringLiteral("JetBrains Mono"), QStringLiteral("Fira Code"),
      QStringLiteral("Source Code Pro"), QStringLiteral("IBM Plex Mono"),
      QStringLiteral("Menlo"), QStringLiteral("Consolas")};
  return chooseFont(monoCandidates, 10, QFont::Medium);
}

QBrush makeGridBrush(const QColor &base, const QColor &minor,
                     const QColor &major) {
  const int size = 64;
  QPixmap pixmap(size, size);
  pixmap.fill(base);
  QPainter painter(&pixmap);
  painter.setRenderHint(QPainter::Antialiasing, false);
  painter.setPen(QPen(minor, 1));
  for (int i = 0; i <= size; i += 16) {
    painter.drawLine(i, 0, i, size);
    painter.drawLine(0, i, size, i);
  }
  painter.setPen(QPen(major, 1));
  for (int i = 0; i <= size; i += 32) {
    painter.drawLine(i, 0, i, size);
    painter.drawLine(0, i, size, i);
  }
  return QBrush(pixmap);
}

void applyDevTheme() {
  const QStringList uiCandidates = {
      QStringLiteral("Space Grotesk"), QStringLiteral("Manrope"),
      QStringLiteral("IBM Plex Sans"), QStringLiteral("Noto Sans"),
      QStringLiteral("Segoe UI"), QStringLiteral("Helvetica Neue")};
  qApp->setFont(chooseFont(uiCandidates, 10, QFont::Normal));

  const QColor windowColor = qApp->palette().color(QPalette::Window);
  const bool useDark = windowColor.lightness() < 128;
  g_isDarkTheme = useDark;
  const QColor base = useDark ? QColor("#0f131a") : QColor("#f7f9fd");
  const QColor minor = useDark ? QColor(28, 38, 51, 130) : QColor(221, 227, 236, 160);
  const QColor major = useDark ? QColor(34, 48, 66, 180) : QColor(206, 214, 226, 180);
  QPalette palette = qApp->palette();
  palette.setBrush(QPalette::Window, makeGridBrush(base, minor, major));
  qApp->setPalette(palette);
  const QString style = useDark ? QStringLiteral(R"(
    QMainWindow {
      background: transparent;
    }
    QWidget {
      color: #e2e7f0;
    }
    QFrame#HeroPanel {
      background: rgba(16, 22, 30, 0.92);
      border: 1px solid #223040;
      border-radius: 14px;
    }
    QLabel#HeroBrand {
      color: #e9edf5;
      font-weight: 700;
      letter-spacing: 1px;
    }
    QLabel#HeroKicker {
      color: #9da9ba;
      font-size: 12px;
    }
    QLabel#HeroTitle {
      color: #f7f9ff;
      font-size: 32px;
      font-weight: 700;
    }
    QLabel#HeroSubtitle {
      color: #b7c2d3;
      font-size: 13px;
    }
    QToolButton[navButton="true"] {
      background: transparent;
      color: #9da9ba;
      border: 1px solid transparent;
      padding: 4px 10px;
      border-radius: 6px;
    }
    QToolButton[navButton="true"]:hover {
      color: #f0f4ff;
      border: 1px solid #2c3b4f;
      background: rgba(26, 36, 50, 0.8);
    }
    QPushButton[outlineButton="true"] {
      background: transparent;
      color: #f0f4ff;
      border: 1px solid #2f3f56;
      border-radius: 8px;
      padding: 7px 16px;
      font-weight: 600;
    }
    QPushButton[outlineButton="true"]:hover {
      border: 1px solid #4aa3ff;
      color: #ffffff;
    }
    QTabWidget::pane {
      background: #1a2230;
      border: 1px solid #263142;
      border-radius: 12px;
      margin-top: 8px;
    }
    QTabBar::tab {
      background: #141b24;
      border: 1px solid #263142;
      border-bottom: none;
      padding: 7px 16px;
      margin-right: 2px;
      border-top-left-radius: 10px;
      border-top-right-radius: 10px;
      color: #aeb8c7;
    }
    QTabBar::tab:selected {
      background: #1a2230;
      color: #f3f7ff;
    }
    QTabBar::tab:hover {
      background: #1c2534;
    }
    QGroupBox[sectionCard="true"] {
      background: #1a2230;
      border: 1px solid #263142;
      border-radius: 12px;
      margin-top: 16px;
    }
    QGroupBox::title {
      subcontrol-origin: margin;
      subcontrol-position: top left;
      left: 12px;
      padding: 0 6px;
      color: #c9d3e2;
      font-weight: 600;
    }
    QLineEdit,
    QComboBox,
    QSpinBox,
    QTextEdit,
    QPlainTextEdit {
      background: #121822;
      border: 1px solid #2b384b;
      border-radius: 8px;
      padding: 6px 10px;
      selection-background-color: #264b7a;
    }
    QLineEdit:focus,
    QComboBox:focus,
    QSpinBox:focus,
    QTextEdit:focus,
    QPlainTextEdit:focus {
      background: #151e2b;
      border: 1px solid #4aa3ff;
    }
    QTableWidget {
      background: #121822;
      border: 1px solid #2b384b;
      border-radius: 8px;
      gridline-color: #2b384b;
    }
    QHeaderView::section {
      background: #1f2937;
      border: 1px solid #2b384b;
      padding: 4px 6px;
      font-weight: 600;
      color: #c9d3e2;
    }
    QComboBox::drop-down {
      border-left: 1px solid #2b384b;
      width: 22px;
    }
    QPushButton {
      background: #2b6de0;
      color: #f3f7ff;
      border: none;
      border-radius: 8px;
      padding: 7px 16px;
      font-weight: 600;
    }
    QPushButton:hover {
      background: #1f57ba;
    }
    QPushButton:pressed {
      background: #164390;
    }
    QToolButton {
      background: #1f2937;
      border: 1px solid #2b384b;
      border-radius: 8px;
      padding: 4px 8px;
      color: #c6d0df;
    }
    QToolButton:hover {
      background: #243243;
    }
    QMenuBar {
      background: #121822;
      border-bottom: 1px solid #263142;
    }
    QMenuBar::item {
      padding: 6px 12px;
      margin: 2px 4px;
      border-radius: 6px;
    }
    QMenuBar::item:selected {
      background: #1f2a3a;
    }
    QMenu {
      background: #121822;
      border: 1px solid #263142;
      padding: 4px;
    }
    QMenu::item {
      padding: 6px 20px;
      border-radius: 6px;
    }
    QMenu::item:selected {
      background: #1f2a3a;
    }
    QCheckBox {
      spacing: 8px;
      color: #c9d3e2;
    }
    QCheckBox::indicator {
      width: 16px;
      height: 16px;
      border-radius: 4px;
      border: 1px solid #2b384b;
      background: #121822;
    }
    QCheckBox::indicator:checked {
      background: #2b6de0;
      border: 1px solid #2b6de0;
    }
    QStatusBar {
      background: #121822;
      border-top: 1px solid #263142;
    }
    QSplitter::handle {
      background: #263142;
    }
    QScrollBar:vertical {
      background: transparent;
      width: 10px;
      margin: 2px;
    }
    QScrollBar::handle:vertical {
      background: #2b384b;
      border-radius: 4px;
      min-height: 30px;
    }
    QScrollBar::add-line:vertical,
    QScrollBar::sub-line:vertical {
      height: 0px;
    }
    QScrollBar:horizontal {
      background: transparent;
      height: 10px;
      margin: 2px;
    }
    QScrollBar::handle:horizontal {
      background: #2b384b;
      border-radius: 4px;
      min-width: 30px;
    }
    QScrollBar::add-line:horizontal,
    QScrollBar::sub-line:horizontal {
      width: 0px;
    }
  )")
                          : QStringLiteral(R"(
    QMainWindow {
      background: transparent;
    }
    QWidget {
      color: #1b1f24;
    }
    QFrame#HeroPanel {
      background: rgba(255, 255, 255, 0.95);
      border: 1px solid #d9e1ee;
      border-radius: 14px;
    }
    QLabel#HeroBrand {
      color: #1b1f24;
      font-weight: 700;
      letter-spacing: 1px;
    }
    QLabel#HeroKicker {
      color: #5b6675;
      font-size: 12px;
    }
    QLabel#HeroTitle {
      color: #0c1118;
      font-size: 32px;
      font-weight: 700;
    }
    QLabel#HeroSubtitle {
      color: #5b6675;
      font-size: 13px;
    }
    QToolButton[navButton="true"] {
      background: transparent;
      color: #4b5563;
      border: 1px solid transparent;
      padding: 4px 10px;
      border-radius: 6px;
    }
    QToolButton[navButton="true"]:hover {
      color: #111827;
      border: 1px solid #d9e1ee;
      background: rgba(236, 242, 251, 0.9);
    }
    QPushButton[outlineButton="true"] {
      background: transparent;
      color: #1b1f24;
      border: 1px solid #c9d3e2;
      border-radius: 8px;
      padding: 7px 16px;
      font-weight: 600;
    }
    QPushButton[outlineButton="true"]:hover {
      border: 1px solid #2c7be5;
      color: #0c1118;
    }
    QTabWidget::pane {
      background: #ffffff;
      border: 1px solid #d9e1ee;
      border-radius: 12px;
      margin-top: 8px;
    }
    QTabBar::tab {
      background: #e9eef6;
      border: 1px solid #d9e1ee;
      border-bottom: none;
      padding: 7px 16px;
      margin-right: 2px;
      border-top-left-radius: 10px;
      border-top-right-radius: 10px;
      color: #3b4552;
    }
    QTabBar::tab:selected {
      background: #ffffff;
      color: #1b1f24;
    }
    QTabBar::tab:hover {
      background: #f2f6fd;
    }
    QGroupBox[sectionCard="true"] {
      background: #ffffff;
      border: 1px solid #d9e1ee;
      border-radius: 12px;
      margin-top: 16px;
    }
    QGroupBox::title {
      subcontrol-origin: margin;
      subcontrol-position: top left;
      left: 12px;
      padding: 0 6px;
      color: #2f3a4a;
      font-weight: 600;
    }
    QLineEdit,
    QComboBox,
    QSpinBox,
    QTextEdit,
    QPlainTextEdit {
      background: #f9fbff;
      border: 1px solid #d3dae6;
      border-radius: 8px;
      padding: 6px 10px;
      selection-background-color: #cfe3ff;
    }
    QLineEdit:focus,
    QComboBox:focus,
    QSpinBox:focus,
    QTextEdit:focus,
    QPlainTextEdit:focus {
      background: #ffffff;
      border: 1px solid #2c7be5;
    }
    QTableWidget {
      background: #ffffff;
      border: 1px solid #d3dae6;
      border-radius: 8px;
      gridline-color: #e1e7f0;
    }
    QHeaderView::section {
      background: #eef2f7;
      border: 1px solid #d3dae6;
      padding: 4px 6px;
      font-weight: 600;
      color: #2f3a4a;
    }
    QComboBox::drop-down {
      border-left: 1px solid #d3dae6;
      width: 22px;
    }
    QPushButton {
      background: #2c7be5;
      color: #ffffff;
      border: none;
      border-radius: 8px;
      padding: 7px 16px;
      font-weight: 600;
    }
    QPushButton:hover {
      background: #2366c4;
    }
    QPushButton:pressed {
      background: #1c529d;
    }
    QToolButton {
      background: #eef2f7;
      border: 1px solid #d3dae6;
      border-radius: 8px;
      padding: 4px 8px;
      color: #394454;
    }
    QToolButton:hover {
      background: #e1e8f3;
    }
    QMenuBar {
      background: #ffffff;
      border-bottom: 1px solid #d9e1ee;
    }
    QMenuBar::item {
      padding: 6px 12px;
      margin: 2px 4px;
      border-radius: 6px;
    }
    QMenuBar::item:selected {
      background: #eaf2ff;
    }
    QMenu {
      background: #ffffff;
      border: 1px solid #d9e1ee;
      padding: 4px;
    }
    QMenu::item {
      padding: 6px 20px;
      border-radius: 6px;
    }
    QMenu::item:selected {
      background: #eaf2ff;
    }
    QCheckBox {
      spacing: 8px;
      color: #2f3a4a;
    }
    QCheckBox::indicator {
      width: 16px;
      height: 16px;
      border-radius: 4px;
      border: 1px solid #c4ccda;
      background: #ffffff;
    }
    QCheckBox::indicator:checked {
      background: #2c7be5;
      border: 1px solid #2c7be5;
    }
    QStatusBar {
      background: #ffffff;
      border-top: 1px solid #d9e1ee;
    }
    QSplitter::handle {
      background: #d9e1ee;
    }
    QScrollBar:vertical {
      background: transparent;
      width: 10px;
      margin: 2px;
    }
    QScrollBar::handle:vertical {
      background: #c9d3e2;
      border-radius: 4px;
      min-height: 30px;
    }
    QScrollBar::add-line:vertical,
    QScrollBar::sub-line:vertical {
      height: 0px;
    }
    QScrollBar:horizontal {
      background: transparent;
      height: 10px;
      margin: 2px;
    }
    QScrollBar::handle:horizontal {
      background: #c9d3e2;
      border-radius: 4px;
      min-width: 30px;
    }
    QScrollBar::add-line:horizontal,
    QScrollBar::sub-line:horizontal {
      width: 0px;
    }
  )");

  qApp->setStyleSheet(style);
}

QString makePythonNotesPath() {
  const QString sessionNumber = QString::number(QDateTime::currentSecsSinceEpoch());
  return QDir::currentPath() + "/cli_calc_notes_" + sessionNumber + ".py";
}

class PythonSyntaxHighlighter : public QSyntaxHighlighter {
public:
  explicit PythonSyntaxHighlighter(QTextDocument *parent,
                                   std::function<bool(int)> allowHighlight)
      : QSyntaxHighlighter(parent),
        allowHighlight_(std::move(allowHighlight)) {
    QTextCharFormat keywordFormat;
    keywordFormat.setForeground(QColor(0, 70, 140));
    keywordFormat.setFontWeight(QFont::Bold);
    const QStringList keywords = {
        "and",   "as",     "assert", "break", "class", "continue", "def",
        "del",   "elif",   "else",   "except","False", "finally",  "for",
        "from",  "global", "if",     "import","in",    "is",       "lambda",
        "None",  "nonlocal","not",   "or",    "pass",  "raise",    "return",
        "True",  "try",    "while",  "with",  "yield"};
    for (const auto &kw : keywords) {
      rules_.push_back({QRegularExpression(QString("\\b%1\\b").arg(kw)), keywordFormat});
    }

    QTextCharFormat stringFormat;
    stringFormat.setForeground(QColor(150, 80, 20));
    rules_.push_back({QRegularExpression(R"('([^'\\]|\\.)*')"), stringFormat});
    rules_.push_back({QRegularExpression(R"("([^"\\]|\\.)*")"), stringFormat});

    QTextCharFormat numberFormat;
    numberFormat.setForeground(QColor(120, 0, 120));
    rules_.push_back({QRegularExpression(R"(\b\d+(\.\d+)?\b)"), numberFormat});

    commentFormat_.setForeground(QColor(110, 110, 110));
    commentFormat_.setFontItalic(true);
    commentRegex_ = QRegularExpression(R"(#.*$)");
  }

protected:
  void highlightBlock(const QString &text) override {
    if (allowHighlight_) {
      const int blockStart = currentBlock().position();
      if (!allowHighlight_(blockStart)) {
        return;
      }
    }
    for (const auto &rule : rules_) {
      QRegularExpressionMatchIterator it = rule.regex.globalMatch(text);
      while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        setFormat(match.capturedStart(), match.capturedLength(), rule.format);
      }
    }
    QRegularExpressionMatch commentMatch = commentRegex_.match(text);
    if (commentMatch.hasMatch()) {
      setFormat(commentMatch.capturedStart(), commentMatch.capturedLength(), commentFormat_);
    }
  }

private:
  struct HighlightRule {
    QRegularExpression regex;
    QTextCharFormat format;
  };
  std::vector<HighlightRule> rules_;
  QRegularExpression commentRegex_;
  QTextCharFormat commentFormat_;
  std::function<bool(int)> allowHighlight_;
};

class BracketPairer : public QObject {
public:
  explicit BracketPairer(QPlainTextEdit *editor,
                         std::function<bool(int)> allowPairing)
      : QObject(editor),
        editor_(editor),
        allowPairing_(std::move(allowPairing)) {}

protected:
  bool eventFilter(QObject *obj, QEvent *event) override {
    if (obj != editor_ || event->type() != QEvent::KeyPress) {
      return QObject::eventFilter(obj, event);
    }
    auto *keyEvent = static_cast<QKeyEvent *>(event);
    const QString text = keyEvent->text();
    if (text.isEmpty() || !editor_) {
      return QObject::eventFilter(obj, event);
    }
    const int cursorPos = editor_->textCursor().position();
    if (allowPairing_ && !allowPairing_(cursorPos)) {
      return QObject::eventFilter(obj, event);
    }
    if (text == "(" || text == "[" || text == "{") {
      const QChar open = text[0];
      const QChar close = (open == '(') ? ')' : (open == '[' ? ']' : '}');
      QTextCursor cursor = editor_->textCursor();
      cursor.beginEditBlock();
      cursor.insertText(QString(open) + close);
      cursor.movePosition(QTextCursor::Left);
      cursor.endEditBlock();
      editor_->setTextCursor(cursor);
      return true;
    }
    if (keyEvent->key() == Qt::Key_Backspace) {
      QTextCursor cursor = editor_->textCursor();
      if (!cursor.hasSelection()) {
        const int pos = cursor.position();
        const QString text = editor_->toPlainText();
        if (pos > 0 && pos < text.size()) {
          if (allowPairing_ && !(allowPairing_(pos) || allowPairing_(pos - 1))) {
            return QObject::eventFilter(obj, event);
          }
          const QChar prev = text.at(pos - 1);
          const QChar next = text.at(pos);
          if ((prev == '(' && next == ')') ||
              (prev == '[' && next == ']') ||
              (prev == '{' && next == '}')) {
            cursor.beginEditBlock();
            cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, 1);
            cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 1);
            cursor.removeSelectedText();
            cursor.endEditBlock();
            editor_->setTextCursor(cursor);
            return true;
          }
        }
      }
    }
    return QObject::eventFilter(obj, event);
  }

private:
  QPlainTextEdit *editor_ = nullptr;
  std::function<bool(int)> allowPairing_;
};

class BracketMatchHighlighter : public QObject {
public:
  explicit BracketMatchHighlighter(QPlainTextEdit *editor,
                                   std::function<bool(int)> allowHighlight)
      : QObject(editor),
        editor_(editor),
        allowHighlight_(std::move(allowHighlight)) {
    if (editor_) {
      connect(editor_, &QPlainTextEdit::cursorPositionChanged, this,
              &BracketMatchHighlighter::updateHighlight);
    }
  }

private:
  static bool isOpenBracket(QChar ch) {
    return ch == '(' || ch == '[' || ch == '{';
  }

  static bool isCloseBracket(QChar ch) {
    return ch == ')' || ch == ']' || ch == '}';
  }

  static QChar matchingBracket(QChar ch) {
    switch (ch.unicode()) {
      case '(':
        return ')';
      case ')':
        return '(';
      case '[':
        return ']';
      case ']':
        return '[';
      case '{':
        return '}';
      case '}':
        return '{';
      default:
        return QChar();
    }
  }

  int findMatchForward(const QString &text, int pos) const {
    const QChar open = text.at(pos);
    const QChar close = matchingBracket(open);
    int depth = 0;
    for (int i = pos + 1; i < text.size(); ++i) {
      const QChar ch = text.at(i);
      if (ch == open) {
        ++depth;
      } else if (ch == close) {
        if (depth == 0) {
          return i;
        }
        --depth;
      }
    }
    return -1;
  }

  int findMatchBackward(const QString &text, int pos) const {
    const QChar close = text.at(pos);
    const QChar open = matchingBracket(close);
    int depth = 0;
    for (int i = pos - 1; i >= 0; --i) {
      const QChar ch = text.at(i);
      if (ch == close) {
        ++depth;
      } else if (ch == open) {
        if (depth == 0) {
          return i;
        }
        --depth;
      }
    }
    return -1;
  }

  void updateHighlight() {
    if (!editor_) {
      return;
    }
    QList<QTextEdit::ExtraSelection> selections;
    const QString text = editor_->toPlainText();
    if (text.isEmpty()) {
      editor_->setExtraSelections(selections);
      return;
    }

    QTextCursor cursor = editor_->textCursor();
    int pos = cursor.position();
    int bracketPos = -1;
    if (pos >= 0 && pos < text.size() && (isOpenBracket(text.at(pos)) ||
                                          isCloseBracket(text.at(pos)))) {
      bracketPos = pos;
    } else if (pos > 0 && (isOpenBracket(text.at(pos - 1)) ||
                           isCloseBracket(text.at(pos - 1)))) {
      bracketPos = pos - 1;
    }

    if (bracketPos == -1) {
      editor_->setExtraSelections(selections);
      return;
    }
    if (allowHighlight_ && !allowHighlight_(bracketPos)) {
      editor_->setExtraSelections(selections);
      return;
    }

    const QChar bracket = text.at(bracketPos);
    int matchPos = -1;
    if (isOpenBracket(bracket)) {
      matchPos = findMatchForward(text, bracketPos);
    } else if (isCloseBracket(bracket)) {
      matchPos = findMatchBackward(text, bracketPos);
    }
    if (matchPos == -1) {
      editor_->setExtraSelections(selections);
      return;
    }
    if (allowHighlight_ && !allowHighlight_(matchPos)) {
      editor_->setExtraSelections(selections);
      return;
    }

    QTextCharFormat format;
    format.setForeground(QColor(180, 0, 0));
    format.setBackground(QColor(255, 220, 220));

    QTextEdit::ExtraSelection first;
    QTextCursor firstCursor(editor_->document());
    firstCursor.setPosition(bracketPos);
    firstCursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 1);
    first.cursor = firstCursor;
    first.format = format;
    selections.append(first);

    QTextEdit::ExtraSelection second;
    QTextCursor secondCursor(editor_->document());
    secondCursor.setPosition(matchPos);
    secondCursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 1);
    second.cursor = secondCursor;
    second.format = format;
    selections.append(second);

    editor_->setExtraSelections(selections);
  }

  QPlainTextEdit *editor_ = nullptr;
  std::function<bool(int)> allowHighlight_;
};
} // namespace

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
  setWindowTitle("CLI Calculator GUI");
  resize(860, 640);
  applyDevTheme();

  auto *fileMenu = menuBar()->addMenu("File");
  auto *settingsMenu = menuBar()->addMenu("Settings");
  auto *helpMenu = menuBar()->addMenu("Help");

  auto *clearOutputAction = fileMenu->addAction("Clear Output");
  auto *clearHistoryAction = fileMenu->addAction("Clear Expression History");
  fileMenu->addSeparator();
  notesNewAction_ = fileMenu->addAction("Notes: New");
  notesOpenAction_ = fileMenu->addAction("Notes: Open");
  notesSaveAction_ = fileMenu->addAction("Notes: Save");
  notesSaveAsAction_ = fileMenu->addAction("Notes: Save As");
  notesCopyAction_ = fileMenu->addAction("Notes: Copy");
  fileMenu->addSeparator();
  auto *exitAction = fileMenu->addAction("Exit");

  auto *defaultBigIntAction = settingsMenu->addAction("Use BigInt by Default");
  defaultBigIntAction->setCheckable(true);

  auto *modeMenu = settingsMenu->addMenu("Mode");
  auto *desktopModeAction = modeMenu->addAction("Desktop");
  auto *touchModeAction = modeMenu->addAction("Touchscreen");
  desktopModeAction->setCheckable(true);
  touchModeAction->setCheckable(true);
  desktopModeAction->setChecked(true);

  auto *matrixEditorAction = settingsMenu->addAction("Enable Matrix Editor");
  matrixEditorAction->setCheckable(true);
  matrixEditorAction->setChecked(true);

  auto *wrapOutputAction = settingsMenu->addAction("Wrap Output Text");
  wrapOutputAction->setCheckable(true);
  wrapOutputAction->setChecked(false);

  auto *splitViewAction = settingsMenu->addAction("Split View");
  splitViewAction->setCheckable(true);
  splitViewAction->setChecked(false);

  auto *toggleNotesEditorAction = settingsMenu->addAction("Toggle Notes Editor");
  toggleNotesEditorAction->setCheckable(true);
  toggleNotesEditorAction->setChecked(true);
  notesTogglePreviewAction_ = settingsMenu->addAction("Toggle Notes Preview");
  notesTogglePreviewAction_->setCheckable(true);
  notesTogglePreviewAction_->setChecked(true);

  auto *aboutAction = helpMenu->addAction("About");
  auto *contributeAction = helpMenu->addAction("Contribute");
  auto *reportBugAction = helpMenu->addAction("Report a Bug");
  auto *requestFeatureAction = helpMenu->addAction("Request a Feature");

  auto *central = new QWidget(this);
  central->setAutoFillBackground(true);
  auto *layout = new QVBoxLayout(central);
  layout->setContentsMargins(16, 16, 16, 16);
  layout->setSpacing(12);

  mainTabs_ = new QTabWidget(central);
  mainContentStack_ = new QStackedWidget(central);
  splitViewContainer_ = new QWidget(central);
  splitLeftStack_ = new QStackedWidget(splitViewContainer_);
  splitRightStack_ = new QStackedWidget(splitViewContainer_);
  tabPool_ = new QStackedWidget(central);
  tabPool_->setVisible(false);

  splitBarContainer_ = new QWidget(central);
  auto *splitBarLayout = new QHBoxLayout(splitBarContainer_);
  splitBarLayout->setContentsMargins(0, 0, 0, 0);
  auto *splitLabel = new QLabel("Split View");
  splitLeftSelect_ = new QComboBox();
  splitRightSelect_ = new QComboBox();
  splitBarLayout->addWidget(splitLabel);
  splitBarLayout->addWidget(new QLabel("Left"));
  splitBarLayout->addWidget(splitLeftSelect_);
  splitBarLayout->addSpacing(12);
  splitBarLayout->addWidget(new QLabel("Right"));
  splitBarLayout->addWidget(splitRightSelect_);
  splitBarLayout->addStretch();
  splitBarContainer_->setVisible(false);

  auto *splitTabsSplitter = new QSplitter(Qt::Horizontal, splitViewContainer_);
  splitTabsSplitter->addWidget(splitLeftStack_);
  splitTabsSplitter->addWidget(splitRightStack_);
  splitTabsSplitter->setStretchFactor(0, 1);
  splitTabsSplitter->setStretchFactor(1, 1);
  auto *splitViewLayout = new QVBoxLayout(splitViewContainer_);
  splitViewLayout->setContentsMargins(0, 0, 0, 0);
  splitViewLayout->addWidget(splitTabsSplitter);

  mainContentStack_->addWidget(mainTabs_);
  mainContentStack_->addWidget(splitViewContainer_);

  auto *expressionTab = new QWidget(mainTabs_);
  {
    auto *tabLayout = new QVBoxLayout(expressionTab);
    auto *formLayout = new QFormLayout();
    exprInput_ = new QLineEdit();
    exprInput_->setPlaceholderText("e.g. (2 + 3) * 4 - sin(1)");
    bigIntCheck_ = new QCheckBox("Use BigInt (integers only)");
    formLayout->addRow("Expression", exprInput_);
    formLayout->addRow("", bigIntCheck_);
    auto *runButton = new QPushButton("Evaluate");
    auto *buttonRow = new QHBoxLayout();
    buttonRow->addStretch();
    buttonRow->addWidget(runButton);
    exprHistory_ = new QPlainTextEdit();
    exprHistory_->setReadOnly(true);
    exprHistory_->setPlaceholderText("Expression history...");
    auto *historyLayout = new QVBoxLayout();
    historyLayout->addWidget(exprHistory_);
    tabLayout->addLayout(formLayout);
    tabLayout->addLayout(buttonRow);
    tabLayout->addWidget(wrapSection("History", historyLayout));
    tabLayout->addStretch();

    connect(runButton, &QPushButton::clicked, this, [this]() {
      const auto expression = exprInput_->text().toStdString();
      const bool useBigInt = bigIntCheck_->isChecked();
      runAndShow("Expression", [expression, useBigInt](OutputFormat format) {
        return runEval(expression, format, nullptr, useBigInt);
      });
      if (exprHistory_) {
        const QString outputText = output_ ? output_->toPlainText().trimmed()
                                           : QString();
        QString entry = QString(">> %1").arg(exprInput_->text().trimmed());
        if (!outputText.isEmpty()) {
          entry += "\n" + outputText;
        }
        exprHistory_->appendPlainText(entry);
        exprHistory_->appendPlainText("");
      }
    });
    connect(exprInput_, &QLineEdit::returnPressed, runButton, &QPushButton::click);
  }
  registerTab("Expressions", expressionTab);

  auto *numberTab = new QWidget(mainTabs_);
  {
    auto *tabLayout = new QVBoxLayout(numberTab);
    auto *numberTabs = new QTabWidget(numberTab);

    auto *sqrtTab = new QWidget(numberTabs);
    auto *sqrtLayout = new QVBoxLayout(sqrtTab);
    auto *sqrtForm = new QFormLayout();
    sqrtInput_ = new QLineEdit();
    sqrtInput_->setPlaceholderText("e.g. 9");
    sqrtForm->addRow("Value", sqrtInput_);
    auto *sqrtButton = new QPushButton("Square Root");
    auto *sqrtRow = new QHBoxLayout();
    sqrtRow->addStretch();
    sqrtRow->addWidget(sqrtButton);
    sqrtForm->addRow("", sqrtRow);
    sqrtLayout->addWidget(wrapSection("Square Root", sqrtForm));
    sqrtLayout->addStretch();
    numberTabs->addTab(sqrtTab, "Square Root");

    auto *divTab = new QWidget(numberTabs);
    auto *divLayout = new QVBoxLayout(divTab);
    auto *divForm = new QFormLayout();
    divisorsInput_ = new QLineEdit();
    divisorsInput_->setPlaceholderText("e.g. 28");
    divForm->addRow("Number", divisorsInput_);
    auto *divButton = new QPushButton("Divisors");
    auto *divRow = new QHBoxLayout();
    divRow->addStretch();
    divRow->addWidget(divButton);
    divForm->addRow("", divRow);
    divLayout->addWidget(wrapSection("Divisors", divForm));
    divLayout->addStretch();
    numberTabs->addTab(divTab, "Divisors");

    auto *primeTab = new QWidget(numberTabs);
    auto *primeLayout = new QVBoxLayout(primeTab);
    auto *primeForm = new QFormLayout();
    primeFactorsInput_ = new QLineEdit();
    primeFactorsInput_->setPlaceholderText("e.g. 84");
    primeForm->addRow("Number", primeFactorsInput_);
    auto *primeButton = new QPushButton("Prime Factorization");
    auto *primeRow = new QHBoxLayout();
    primeRow->addStretch();
    primeRow->addWidget(primeButton);
    primeForm->addRow("", primeRow);
    primeLayout->addWidget(wrapSection("Prime Factors", primeForm));
    primeLayout->addStretch();
    numberTabs->addTab(primeTab, "Prime Factors");

    tabLayout->addWidget(numberTabs);
    tabLayout->addStretch();

    connect(sqrtButton, &QPushButton::clicked, this, [this]() {
      const auto value = sqrtInput_->text().toStdString();
      runAndShow("Square Root", [value](OutputFormat format) {
        return runSquareRoot(value, format, nullptr);
      });
    });
    connect(sqrtInput_, &QLineEdit::returnPressed, sqrtButton, &QPushButton::click);

    connect(divButton, &QPushButton::clicked, this, [this]() {
      const auto value = divisorsInput_->text().toStdString();
      runAndShow("Divisors", [value](OutputFormat format) {
        return runDivisors(value, format);
      });
    });
    connect(divisorsInput_, &QLineEdit::returnPressed, divButton, &QPushButton::click);

    connect(primeButton, &QPushButton::clicked, this, [this]() {
      const auto value = primeFactorsInput_->text().toStdString();
      runAndShow("Prime Factors", [value](OutputFormat format) {
        return runPrimeFactorization(value, format);
      });
    });
    connect(primeFactorsInput_, &QLineEdit::returnPressed, primeButton, &QPushButton::click);
  }
  registerTab("Numbers", numberTab);

  auto *convertTab = new QWidget(mainTabs_);
  {
    auto *tabLayout = new QVBoxLayout(convertTab);
    auto *convertTabs = new QTabWidget(convertTab);

    auto *baseTab = new QWidget(convertTabs);
    auto *baseLayout = new QVBoxLayout(baseTab);
    auto *baseForm = new QFormLayout();
    baseFromInput_ = new QComboBox();
    baseToInput_ = new QComboBox();
    const std::vector<std::pair<QString, QString>> baseOptions = {
        {QStringLiteral("Binary (2)"), QStringLiteral("2")},
        {QStringLiteral("Decimal (10)"), QStringLiteral("10")},
        {QStringLiteral("Hex (16)"), QStringLiteral("16")},
    };
    for (const auto &option : baseOptions) {
      baseFromInput_->addItem(option.first, option.second);
      baseToInput_->addItem(option.first, option.second);
    }
    baseValueInput_ = new QLineEdit();
    baseValueInput_->setPlaceholderText("Value to convert");
    baseForm->addRow("From base", baseFromInput_);
    baseForm->addRow("To base", baseToInput_);
    baseForm->addRow("Value", baseValueInput_);
    auto *baseButton = new QPushButton("Convert Bases");
    auto *baseRow = new QHBoxLayout();
    baseRow->addStretch();
    baseRow->addWidget(baseButton);
    baseForm->addRow("", baseRow);
    baseLayout->addWidget(wrapSection("Base Conversion", baseForm));
    baseLayout->addStretch();
    convertTabs->addTab(baseTab, "Base Conversion");

    auto *unitTab = new QWidget(convertTabs);
    auto *unitLayout = new QVBoxLayout(unitTab);
    auto *unitForm = new QFormLayout();
    unitCategoryInput_ = new QComboBox();
    unitFromInput_ = new QComboBox();
    unitToInput_ = new QComboBox();
    unitCategoryInput_->addItem("Length", "length");
    unitCategoryInput_->addItem("Mass", "mass");
    unitCategoryInput_->addItem("Volume", "volume");
    unitCategoryInput_->addItem("Temperature", "temperature");
    unitValueInput_ = new QLineEdit();
    unitForm->addRow("Category", unitCategoryInput_);
    unitForm->addRow("From unit", unitFromInput_);
    unitForm->addRow("To unit", unitToInput_);
    unitForm->addRow("Value", unitValueInput_);
    auto *unitButton = new QPushButton("Convert Units");
    auto *unitRow = new QHBoxLayout();
    unitRow->addStretch();
    unitRow->addWidget(unitButton);
    unitForm->addRow("", unitRow);
    unitLayout->addWidget(wrapSection("Unit Conversion", unitForm));
    unitLayout->addStretch();
    convertTabs->addTab(unitTab, "Unit Conversion");

    tabLayout->addWidget(convertTabs);
    tabLayout->addStretch();

    auto refreshUnits = [this]() {
      unitFromInput_->clear();
      unitToInput_->clear();
      const QString categoryToken = unitCategoryInput_->currentData().toString();
      if (categoryToken == "temperature") {
        for (const auto &unit : temperatureUnits()) {
          const QString label = QString::fromStdString(unit.name + " (" + unit.symbol + ")");
          const QString token = QString::fromStdString(unit.symbol);
          unitFromInput_->addItem(label, token);
          unitToInput_->addItem(label, token);
        }
        return;
      }
      for (const auto &category : linearCategories()) {
        if (toLower(category.name) == categoryToken.toStdString()) {
          for (const auto &unit : category.units) {
            const QString label = QString::fromStdString(unit.name + " (" + unit.symbol + ")");
            const QString token = QString::fromStdString(unit.symbol);
            unitFromInput_->addItem(label, token);
            unitToInput_->addItem(label, token);
          }
          break;
        }
      }
    };
    refreshUnits();

    connect(unitCategoryInput_, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [refreshUnits](int) { refreshUnits(); });

    connect(baseButton, &QPushButton::clicked, this, [this]() {
      const auto fromBase = baseFromInput_->currentData().toString().toStdString();
      const auto toBase = baseToInput_->currentData().toString().toStdString();
      const auto value = baseValueInput_->text().toStdString();
      runAndShow("Base Conversion", [fromBase, toBase, value](OutputFormat format) {
        return runConvert(fromBase, toBase, value, format);
      });
    });
    connect(baseValueInput_, &QLineEdit::returnPressed, baseButton, &QPushButton::click);

    connect(unitButton, &QPushButton::clicked, this, [this]() {
      const auto category = unitCategoryInput_->currentData().toString().toStdString();
      const auto fromUnit = unitFromInput_->currentData().toString().toStdString();
      const auto toUnit = unitToInput_->currentData().toString().toStdString();
      const auto value = unitValueInput_->text().toStdString();
      runAndShow("Unit Conversion", [category, fromUnit, toUnit, value](OutputFormat format) {
        return runUnitConvert(category, fromUnit, toUnit, value, format);
      });
    });
    connect(unitValueInput_, &QLineEdit::returnPressed, unitButton, &QPushButton::click);
  }
  registerTab("Conversions", convertTab);

  auto *equationsTab = new QWidget(mainTabs_);
  {
    auto *tabLayout = new QVBoxLayout(equationsTab);
    auto *equationTabs = new QTabWidget(equationsTab);

    auto *linearTab = new QWidget(equationTabs);
    auto *linearLayout = new QVBoxLayout(linearTab);
    auto *linearForm = new QFormLayout();
    linearAInput_ = new QLineEdit();
    linearBInput_ = new QLineEdit();
    linearForm->addRow("a", linearAInput_);
    linearForm->addRow("b", linearBInput_);
    auto *linearButton = new QPushButton("Solve Linear");
    auto *linearRow = new QHBoxLayout();
    linearRow->addStretch();
    linearRow->addWidget(linearButton);
    linearForm->addRow("", linearRow);
    linearLayout->addWidget(wrapSection("Linear: a*x + b = 0", linearForm));
    linearLayout->addStretch();
    equationTabs->addTab(linearTab, "Linear");

    auto *quadTab = new QWidget(equationTabs);
    auto *quadLayout = new QVBoxLayout(quadTab);
    auto *quadForm = new QFormLayout();
    quadraticAInput_ = new QLineEdit();
    quadraticBInput_ = new QLineEdit();
    quadraticCInput_ = new QLineEdit();
    quadForm->addRow("a", quadraticAInput_);
    quadForm->addRow("b", quadraticBInput_);
    quadForm->addRow("c", quadraticCInput_);
    auto *quadButton = new QPushButton("Solve Quadratic");
    auto *quadRow = new QHBoxLayout();
    quadRow->addStretch();
    quadRow->addWidget(quadButton);
    quadForm->addRow("", quadRow);
    quadLayout->addWidget(wrapSection("Quadratic: a*x^2 + b*x + c = 0", quadForm));
    quadLayout->addStretch();
    equationTabs->addTab(quadTab, "Quadratic");

    tabLayout->addWidget(equationTabs);
    tabLayout->addStretch();

    connect(linearButton, &QPushButton::clicked, this, [this]() {
      const auto aValue = linearAInput_->text().toStdString();
      const auto bValue = linearBInput_->text().toStdString();
      runAndShow("Solve Linear", [aValue, bValue](OutputFormat format) {
        return runSolveLinear(aValue, bValue, format);
      });
    });
    connect(linearAInput_, &QLineEdit::returnPressed, linearButton, &QPushButton::click);
    connect(linearBInput_, &QLineEdit::returnPressed, linearButton, &QPushButton::click);

    connect(quadButton, &QPushButton::clicked, this, [this]() {
      const auto aValue = quadraticAInput_->text().toStdString();
      const auto bValue = quadraticBInput_->text().toStdString();
      const auto cValue = quadraticCInput_->text().toStdString();
      runAndShow("Solve Quadratic", [aValue, bValue, cValue](OutputFormat format) {
        return runSolveQuadratic(aValue, bValue, cValue, format);
      });
    });
    connect(quadraticAInput_, &QLineEdit::returnPressed, quadButton, &QPushButton::click);
    connect(quadraticBInput_, &QLineEdit::returnPressed, quadButton, &QPushButton::click);
    connect(quadraticCInput_, &QLineEdit::returnPressed, quadButton, &QPushButton::click);
  }
  registerTab("Equations", equationsTab);

  auto *matrixTab = new QWidget(mainTabs_);
  {
    auto *tabLayout = new QVBoxLayout(matrixTab);
    matrixInputStack_ = new QStackedWidget();

    auto *matrixEditorWidget = new QWidget(matrixInputStack_);
    auto *editorLayout = new QVBoxLayout(matrixEditorWidget);
    matrixAEditor_ = new MatrixEditor(matrixEditorWidget);
    matrixBEditor_ = new MatrixEditor(matrixEditorWidget);
    auto *matrixALayout = new QVBoxLayout();
    matrixALayout->addWidget(matrixAEditor_);
    auto *matrixBLayout = new QVBoxLayout();
    matrixBLayout->addWidget(matrixBEditor_);
    editorLayout->addWidget(wrapSection("Matrix A", matrixALayout));
    editorLayout->addWidget(wrapSection("Matrix B", matrixBLayout));
    matrixEditorWidget->setLayout(editorLayout);

    auto *matrixTextWidget = new QWidget(matrixInputStack_);
    auto *matrixForm = new QFormLayout(matrixTextWidget);
    matrixAInput_ = new QLineEdit();
    matrixAInput_->setPlaceholderText("Rows ';', columns ',' or spaces");
    matrixBInput_ = new QLineEdit();
    matrixBInput_->setPlaceholderText("Rows ';', columns ',' or spaces");
    matrixForm->addRow("Matrix A", matrixAInput_);
    matrixForm->addRow("Matrix B", matrixBInput_);
    matrixTextWidget->setLayout(matrixForm);

    matrixInputStack_->addWidget(matrixEditorWidget);
    matrixInputStack_->addWidget(matrixTextWidget);
    matrixInputStack_->setCurrentIndex(0);

    auto *buttonsRow = new QHBoxLayout();
    auto *addButton = new QPushButton("Add");
    auto *subButton = new QPushButton("Subtract");
    auto *mulButton = new QPushButton("Multiply");
    buttonsRow->addStretch();
    buttonsRow->addWidget(addButton);
    buttonsRow->addWidget(subButton);
    buttonsRow->addWidget(mulButton);

    auto *matrixGroupLayout = new QVBoxLayout();
    matrixGroupLayout->addWidget(matrixInputStack_);
    matrixGroupLayout->addLayout(buttonsRow);
    tabLayout->addWidget(wrapSection("Matrix Operations", matrixGroupLayout));
    tabLayout->addStretch();

    connect(addButton, &QPushButton::clicked, this, [this]() {
      const auto matrixA = matrixInputStack_->currentIndex() == 0
                               ? matrixAEditor_->toCliString()
                               : matrixAInput_->text().toStdString();
      const auto matrixB = matrixInputStack_->currentIndex() == 0
                               ? matrixBEditor_->toCliString()
                               : matrixBInput_->text().toStdString();
      runAndShow("Matrix Add", [matrixA, matrixB](OutputFormat format) {
        return runMatrixAdd(matrixA, matrixB, format);
      });
    });

    connect(subButton, &QPushButton::clicked, this, [this]() {
      const auto matrixA = matrixInputStack_->currentIndex() == 0
                               ? matrixAEditor_->toCliString()
                               : matrixAInput_->text().toStdString();
      const auto matrixB = matrixInputStack_->currentIndex() == 0
                               ? matrixBEditor_->toCliString()
                               : matrixBInput_->text().toStdString();
      runAndShow("Matrix Subtract", [matrixA, matrixB](OutputFormat format) {
        return runMatrixSubtract(matrixA, matrixB, format);
      });
    });

    connect(mulButton, &QPushButton::clicked, this, [this]() {
      const auto matrixA = matrixInputStack_->currentIndex() == 0
                               ? matrixAEditor_->toCliString()
                               : matrixAInput_->text().toStdString();
      const auto matrixB = matrixInputStack_->currentIndex() == 0
                               ? matrixBEditor_->toCliString()
                               : matrixBInput_->text().toStdString();
      runAndShow("Matrix Multiply", [matrixA, matrixB](OutputFormat format) {
        return runMatrixMultiply(matrixA, matrixB, format);
      });
    });
    connect(matrixAInput_, &QLineEdit::returnPressed, addButton, &QPushButton::click);
    connect(matrixBInput_, &QLineEdit::returnPressed, addButton, &QPushButton::click);
  }
  registerTab("Matrices", matrixTab);

  auto *graphTab = new QWidget(mainTabs_);
  {
    auto *tabLayout = new QVBoxLayout(graphTab);
    auto *graphTabs = new QTabWidget(graphTab);

    auto *manualTab = new QWidget(graphTabs);
    auto *manualLayout = new QVBoxLayout(manualTab);
    auto *valuesForm = new QFormLayout();
    graphValuesInput_ = new QLineEdit();
    graphValuesInput_->setPlaceholderText("e.g. 1 2 3 4 5");
    graphValuesHeightSpin_ = new QSpinBox();
    graphValuesHeightSpin_->setRange(2, 40);
    graphValuesHeightSpin_->setValue(10);
    valuesForm->addRow("Values", graphValuesInput_);
    valuesForm->addRow("Height", graphValuesHeightSpin_);
    auto *valuesButton = new QPushButton("Generate from Values");
    auto *valuesRow = new QHBoxLayout();
    valuesRow->addStretch();
    valuesRow->addWidget(valuesButton);
    valuesForm->addRow("", valuesRow);
    manualLayout->addWidget(wrapSection("Manual Values", valuesForm));
    manualLayout->addStretch();
    graphTabs->addTab(manualTab, "Manual Values");

    auto *csvTab = new QWidget(graphTabs);
    auto *csvLayout = new QVBoxLayout(csvTab);
    auto *csvForm = new QFormLayout();
    graphCsvPathInput_ = new QLineEdit();
    graphCsvPathInput_->setPlaceholderText("Path to CSV file");
    auto *csvBrowseButton = new QPushButton("Browse");
    auto *csvPathRow = new QHBoxLayout();
    csvPathRow->addWidget(graphCsvPathInput_);
    csvPathRow->addWidget(csvBrowseButton);
    csvForm->addRow("CSV path", csvPathRow);
    graphCsvColumnInput_ = new QLineEdit();
    graphCsvColumnInput_->setPlaceholderText("Column name or index (1-based)");
    graphCsvHeightSpin_ = new QSpinBox();
    graphCsvHeightSpin_->setRange(2, 40);
    graphCsvHeightSpin_->setValue(10);
    graphCsvHeadersCheck_ = new QCheckBox("CSV includes headers");
    graphCsvHeadersCheck_->setChecked(true);
    csvForm->addRow("Column", graphCsvColumnInput_);
    csvForm->addRow("Height", graphCsvHeightSpin_);
    csvForm->addRow("", graphCsvHeadersCheck_);
    auto *csvButton = new QPushButton("Generate from CSV");
    auto *csvRow = new QHBoxLayout();
    csvRow->addStretch();
    csvRow->addWidget(csvButton);
    csvForm->addRow("", csvRow);
    csvLayout->addWidget(wrapSection("Import from CSV", csvForm));
    csvLayout->addStretch();
    graphTabs->addTab(csvTab, "Import from CSV");

    tabLayout->addWidget(graphTabs);

    graphPreviewLabel_ = new QLabel("No graph generated yet.");
    graphPreviewLabel_->setAlignment(Qt::AlignCenter);
    graphPreviewLabel_->setMinimumHeight(220);
    graphPreviewLabel_->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    graphPreviewLabel_->setScaledContents(true);
    auto *saveGraphButton = new QPushButton("Save As...");
    auto *previewRow = new QHBoxLayout();
    previewRow->addStretch();
    previewRow->addWidget(saveGraphButton);

    auto *previewLayout = new QVBoxLayout();
    previewLayout->addWidget(graphPreviewLabel_);
    previewLayout->addLayout(previewRow);
    tabLayout->addWidget(wrapSection("Graph Preview", previewLayout));

    tabLayout->addStretch();

    connect(csvBrowseButton, &QPushButton::clicked, this, [this]() {
      const QString path = QFileDialog::getOpenFileName(
          this, "Select CSV", QString(), "CSV Files (*.csv);;All Files (*)");
      if (!path.isEmpty()) {
        graphCsvPathInput_->setText(path);
      }
    });

    connect(valuesButton, &QPushButton::clicked, this, [this]() {
      const QString valuesText = graphValuesInput_->text();
      const auto tokens = splitTokens(valuesText);
      if (tokens.empty()) {
        showGraphError("Graph Values", "Provide at least one value.");
        return;
      }
      std::vector<std::string> args;
      const QString outputPath = makeTempGraphPath();
      args.push_back(outputPath.toStdString());
      args.insert(args.end(), tokens.begin(), tokens.end());
      args.push_back("--height");
      args.push_back(std::to_string(graphValuesHeightSpin_->value()));
      CliRunResult result = runCliAction([&]() {
        return runGraphValues(args, OutputFormat::Text);
      });
      if (result.exitCode != 0) {
        const std::string combined = result.err + result.out;
        showGraphError("Graph Values", QString::fromStdString(combined));
        return;
      }
      showGraphPreview("Graph Values", outputPath);
    });

    connect(csvButton, &QPushButton::clicked, this, [this]() {
      const QString csvPath = graphCsvPathInput_->text().trimmed();
      const QString column = graphCsvColumnInput_->text().trimmed();
      if (csvPath.isEmpty() || column.isEmpty()) {
        showGraphError("Graph CSV", "CSV path and column are required.");
        return;
      }
      std::vector<std::string> args;
      const QString outputPath = makeTempGraphPath();
      args.push_back(outputPath.toStdString());
      args.push_back(csvPath.toStdString());
      args.push_back(column.toStdString());
      args.push_back("--height");
      args.push_back(std::to_string(graphCsvHeightSpin_->value()));
      if (!graphCsvHeadersCheck_->isChecked()) {
        args.push_back("--no-headers");
      }
      CliRunResult result = runCliAction([&]() {
        return runGraphCsv(args, OutputFormat::Text);
      });
      if (result.exitCode != 0) {
        const std::string combined = result.err + result.out;
        showGraphError("Graph CSV", QString::fromStdString(combined));
        return;
      }
      showGraphPreview("Graph CSV", outputPath);
    });
    connect(graphValuesInput_, &QLineEdit::returnPressed, valuesButton, &QPushButton::click);
    connect(graphCsvPathInput_, &QLineEdit::returnPressed, csvButton, &QPushButton::click);
    connect(graphCsvColumnInput_, &QLineEdit::returnPressed, csvButton, &QPushButton::click);

    connect(saveGraphButton, &QPushButton::clicked, this, [this]() {
      if (currentGraphPath_.isEmpty()) {
        statusBar()->showMessage("No graph to save yet", 3000);
        return;
      }
      QString path = QFileDialog::getSaveFileName(
          this, "Save Graph", "graph.png", "PNG Image (*.png)");
      if (path.isEmpty()) {
        return;
      }
      if (!path.endsWith(".png", Qt::CaseInsensitive)) {
        path += ".png";
      }
      if (QFile::exists(path)) {
        QFile::remove(path);
      }
      if (!QFile::copy(currentGraphPath_, path)) {
        statusBar()->showMessage("Failed to save graph", 3000);
        return;
      }
      statusBar()->showMessage("Graph saved", 3000);
    });
  }
  registerTab("Graphs", graphTab);

  auto *statsTab = new QWidget(mainTabs_);
  {
    auto *tabLayout = new QVBoxLayout(statsTab);
    auto *statsForm = new QFormLayout();
    statsInput_ = new QLineEdit();
    statsInput_->setPlaceholderText("Values separated by spaces");
    statsForm->addRow("Values", statsInput_);
    auto *statsButton = new QPushButton("Statistics");
    auto *statsRow = new QHBoxLayout();
    statsRow->addStretch();
    statsRow->addWidget(statsButton);
    statsForm->addRow("", statsRow);
    tabLayout->addWidget(wrapSection("Summary Statistics", statsForm));
    tabLayout->addStretch();

    connect(statsButton, &QPushButton::clicked, this, [this]() {
      const auto tokens = splitTokens(statsInput_->text());
      runAndShow("Statistics", [tokens](OutputFormat format) {
        return runStatistics(tokens, format);
      });
    });
    connect(statsInput_, &QLineEdit::returnPressed, statsButton, &QPushButton::click);
  }
  registerTab("Statistics", statsTab);

  auto *varsTab = new QWidget(mainTabs_);
  {
    auto *tabLayout = new QVBoxLayout(varsTab);
    auto *varsTabs = new QTabWidget(varsTab);

    auto *listTab = new QWidget(varsTabs);
    auto *listLayout = new QVBoxLayout(listTab);
    auto *listButton = new QPushButton("List Variables");
    auto *listSectionLayout = new QVBoxLayout();
    listSectionLayout->addWidget(listButton);
    listLayout->addWidget(wrapSection("List", listSectionLayout));
    listLayout->addStretch();
    varsTabs->addTab(listTab, "List");

    auto *setTab = new QWidget(varsTabs);
    auto *setLayout = new QVBoxLayout(setTab);
    auto *setForm = new QFormLayout();
    varNameInput_ = new QLineEdit();
    varValueInput_ = new QLineEdit();
    setForm->addRow("Name", varNameInput_);
    setForm->addRow("Value", varValueInput_);
    auto *setButton = new QPushButton("Set Variable");
    auto *setRow = new QHBoxLayout();
    setRow->addStretch();
    setRow->addWidget(setButton);
    setForm->addRow("", setRow);
    setLayout->addWidget(wrapSection("Set", setForm));
    setLayout->addStretch();
    varsTabs->addTab(setTab, "Set");

    auto *unsetTab = new QWidget(varsTabs);
    auto *unsetLayout = new QVBoxLayout(unsetTab);
    auto *unsetForm = new QFormLayout();
    unsetVarInput_ = new QLineEdit();
    unsetForm->addRow("Name", unsetVarInput_);
    auto *unsetButton = new QPushButton("Unset Variable");
    auto *unsetRow = new QHBoxLayout();
    unsetRow->addStretch();
    unsetRow->addWidget(unsetButton);
    unsetForm->addRow("", unsetRow);
    unsetLayout->addWidget(wrapSection("Unset", unsetForm));
    unsetLayout->addStretch();
    varsTabs->addTab(unsetTab, "Unset");

    tabLayout->addWidget(varsTabs);
    tabLayout->addStretch();

    connect(listButton, &QPushButton::clicked, this, [this]() {
      runAndShow("Variables", [](OutputFormat format) { return runListVariables(format); });
    });

    connect(setButton, &QPushButton::clicked, this, [this]() {
      const auto name = varNameInput_->text().toStdString();
      const auto value = varValueInput_->text().toStdString();
      runAndShow("Set Variable", [name, value](OutputFormat format) {
        return runSetVariable(name, value, format);
      });
    });
    connect(varNameInput_, &QLineEdit::returnPressed, setButton, &QPushButton::click);
    connect(varValueInput_, &QLineEdit::returnPressed, setButton, &QPushButton::click);

    connect(unsetButton, &QPushButton::clicked, this, [this]() {
      const auto name = unsetVarInput_->text().toStdString();
      runAndShow("Unset Variable", [name](OutputFormat format) {
        return runUnsetVariable(name, format);
      });
    });
    connect(unsetVarInput_, &QLineEdit::returnPressed, unsetButton, &QPushButton::click);
  }
  registerTab("Variables", varsTab);

  auto *terminalTab = new QWidget(mainTabs_);
  {
    auto *tabLayout = new QVBoxLayout(terminalTab);
    terminalOutput_ = new QPlainTextEdit(terminalTab);
    terminalOutput_->setReadOnly(true);
    terminalOutput_->setFont(devMonoFont());
    terminalOutput_->setPlaceholderText("Terminal output...");

    terminalInput_ = new QLineEdit(terminalTab);
    terminalInput_->setPlaceholderText("Enter a command and press Enter");

    auto *inputRow = new QHBoxLayout();
    auto *runButton = new QPushButton("Run");
    inputRow->addWidget(terminalInput_);
    inputRow->addWidget(runButton);

    tabLayout->addWidget(terminalOutput_);
    tabLayout->addLayout(inputRow);

    terminalCwd_ = QDir::currentPath();
    terminalOutput_->appendPlainText(
        "Welcome to the CLI Calculator terminal.\n"
        "Run calculator commands (e.g. calculator --help) or any shell command.\n"
        "Working directory: " + terminalCwd_ + "\n");

    auto runCommand = [this]() {
      const QString command = terminalInput_->text().trimmed();
      if (command.isEmpty()) {
        return;
      }
      terminalOutput_->appendPlainText(
          QString("%1$ %2").arg(terminalCwd_, command));
      terminalInput_->clear();

      if (command.startsWith("cd ")) {
        QString target = command.mid(3).trimmed();
        if (target.isEmpty()) {
          target = QDir::homePath();
        }
        QDir dir(terminalCwd_);
        if (dir.cd(target)) {
          terminalCwd_ = dir.absolutePath();
        } else {
          terminalOutput_->appendPlainText("cd: no such directory\n");
        }
        return;
      }
      if (command == "cd") {
        terminalCwd_ = QDir::homePath();
        return;
      }

      QProcess process;
      process.setWorkingDirectory(terminalCwd_);
      process.start("/bin/bash", QStringList() << "-lc" << command);
      if (!process.waitForFinished(-1)) {
        terminalOutput_->appendPlainText("Command failed to start.\n");
        return;
      }
      const QString stdoutText = QString::fromUtf8(process.readAllStandardOutput());
      const QString stderrText = QString::fromUtf8(process.readAllStandardError());
      if (!stdoutText.isEmpty()) {
        terminalOutput_->appendPlainText(stdoutText);
      }
      if (!stderrText.isEmpty()) {
        terminalOutput_->appendPlainText(stderrText);
      }
    };

    connect(runButton, &QPushButton::clicked, this, runCommand);
    connect(terminalInput_, &QLineEdit::returnPressed, this, runCommand);
  }
  registerTab("Terminal", terminalTab);

  auto *notesTab = new QWidget(mainTabs_);
  {
    auto *tabLayout = new QVBoxLayout(notesTab);
    auto *headerLayout = new QHBoxLayout();
    auto *newButton = new QPushButton("New");
    auto *openButton = new QPushButton("Open");
    auto *saveButton = new QPushButton("Save");
    auto *saveAsButton = new QPushButton("Save As");
    auto *copyButton = new QPushButton("Copy");
    auto *clearNotesOutputButton = new QPushButton("Clear Output");
    notesRunButton_ = new QPushButton("Run");
    notesRunButton_->setEnabled(false);
    notesSwitchButton_ = new QPushButton("Switch to Python");
    headerLayout->addWidget(newButton);
    headerLayout->addWidget(openButton);
    headerLayout->addWidget(saveButton);
    headerLayout->addWidget(saveAsButton);
    headerLayout->addWidget(copyButton);
    headerLayout->addWidget(clearNotesOutputButton);
    headerLayout->addWidget(notesRunButton_);
    headerLayout->addWidget(notesSwitchButton_);
    headerLayout->addStretch();
    tabLayout->addLayout(headerLayout);

    notesSplitter_ = new QSplitter(Qt::Horizontal, notesTab);
    notesEditor_ = new QPlainTextEdit(notesSplitter_);
    notesEditor_->setPlaceholderText("Write notes in Markdown...");
    notesPreview_ = new QTextBrowser(notesSplitter_);
    notesPreview_->setOpenExternalLinks(true);
    notesPreview_->setOpenLinks(false);
    notesSplitter_->addWidget(notesEditor_);
    notesSplitter_->addWidget(notesPreview_);
    notesSplitter_->setStretchFactor(0, 1);
    notesSplitter_->setStretchFactor(1, 1);
    tabLayout->addWidget(notesSplitter_);

    notesHighlighter_ = new PythonSyntaxHighlighter(
        notesEditor_->document(), [this](int pos) {
          return isNotesPythonPosition(pos);
        });
    auto *notesBracketPairer = new BracketPairer(
        notesEditor_, [this](int pos) { return isNotesCodePosition(pos); });
    notesEditor_->installEventFilter(notesBracketPairer);
    auto *notesBracketHighlighter = new BracketMatchHighlighter(
        notesEditor_, [this](int pos) { return isNotesCodePosition(pos); });
    Q_UNUSED(notesBracketHighlighter);

    auto makeNotesPath = []() {
      const QString sessionNumber =
          QString::number(QDateTime::currentSecsSinceEpoch());
      return QDir::currentPath() + "/cli_calc_notes_" + sessionNumber + ".md";
    };
    notesPath_ = makeNotesPath();

    auto refreshNotesActions = [this]() {
      if (notesRefreshInProgress_) {
        return;
      }
      notesRefreshInProgress_ = true;
      updateNotesRanges();
      if (notesSwitchButton_) {
        const bool isPython = isPythonNotesFile(notesPath_);
        notesSwitchButton_->setText(isPython ? "Switch to Markdown" : "Switch to Python");
      }
      if (notesRunButton_) {
        const QString text = notesEditor_ ? notesEditor_->toPlainText() : QString();
        const bool hasBlocks = !extractPythonBlocks(text).empty();
        const bool hasPython = hasBlocks || (isPythonNotesFile(notesPath_) && !text.trimmed().isEmpty());
        notesRunButton_->setEnabled(hasPython);
        if (!hasPython && !notesRunOutput_.isEmpty()) {
          notesRunOutput_.clear();
        }
      }
      if (notesHighlighter_) {
        notesHighlighter_->rehighlight();
      }
      notesRefreshInProgress_ = false;
    };

    connect(notesEditor_, &QPlainTextEdit::textChanged, this, [this, refreshNotesActions]() {
      updateNotesPreview();
      refreshNotesActions();
    });

    connect(notesPreview_, &QTextBrowser::anchorClicked, this, [this](const QUrl &url) {
      const QString scheme = url.scheme();
      if (scheme != "copy" && scheme != "run") {
        QDesktopServices::openUrl(url);
        return;
      }
      int index = -1;
      if (!parseCodeBlockIndex(url, &index) ||
          index < 0 || index >= static_cast<int>(notesCodeBlocks_.size())) {
        statusBar()->showMessage("Invalid code block link", 2000);
        return;
      }
      const auto &block = notesCodeBlocks_[index];
      if (scheme == "copy") {
        QApplication::clipboard()->setText(block.code);
        statusBar()->showMessage("Code copied", 2000);
        return;
      }
      const QString lang = block.language.trimmed().toLower();
      if (!lang.isEmpty() && lang != "py" && lang != "python") {
        statusBar()->showMessage("Run supports Python blocks only", 3000);
        return;
      }
      const PythonRunResult runResult = runPythonSnippet(block.code);
      const bool success = runResult.started && runResult.exitCode == 0;
      if (output_) {
        output_->setPlainText(success ? "Python: Success" : "Python: Fail");
      }
      statusBar()->showMessage(
          success ? "Python run succeeded" : "Python run failed", 3000);
      if (!runResult.started) {
        notesRunOutput_.clear();
        notesCodeBlocks_[index].output = runResult.errorMessage;
      } else {
        const QString combined =
            (runResult.stdoutText + runResult.stderrText).trimmed();
        notesRunOutput_.clear();
        notesCodeBlocks_[index].output = combined.isEmpty() ? "No output." : combined;
      }
      updateNotesPreview();
    });
    updateNotesPreview();

    auto copyNotes = [this]() {
      QApplication::clipboard()->setText(notesEditor_->toPlainText());
      statusBar()->showMessage("Notes copied", 3000);
    };

    auto openNotes = [this, refreshNotesActions]() {
      QString path = QFileDialog::getOpenFileName(
          this, "Open Notes", QDir::currentPath(),
          "Markdown (*.md *.markdown);;Text Files (*.txt);;All Files (*)");
      if (path.isEmpty()) {
        return;
      }
      QFile file(path);
      if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        statusBar()->showMessage("Failed to open notes", 3000);
        return;
      }
      notesEditor_->setPlainText(QString::fromUtf8(file.readAll()));
      notesPath_ = path;
      refreshNotesActions();
      updateNotesPreview();
      statusBar()->showMessage("Notes loaded", 3000);
    };

    auto saveNotes = [this]() {
      QFile file(notesPath_);
      if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        statusBar()->showMessage("Failed to save notes", 3000);
        return;
      }
      const QByteArray data = notesEditor_->toPlainText().toUtf8();
      file.write(data);
      statusBar()->showMessage("Notes saved", 3000);
    };

    auto saveNotesAs = [this]() {
      QString path = QFileDialog::getSaveFileName(
          this, "Save Notes As", notesPath_, "Markdown (*.md);;All Files (*)");
      if (path.isEmpty()) {
        return;
      }
      if (!path.endsWith(".md", Qt::CaseInsensitive)) {
        path += ".md";
      }
      notesPath_ = path;
      QFile file(notesPath_);
      if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        statusBar()->showMessage("Failed to save notes", 3000);
        return;
      }
      const QByteArray data = notesEditor_->toPlainText().toUtf8();
      file.write(data);
      statusBar()->showMessage("Notes saved", 3000);
    };

    auto newNotes = [this, makeNotesPath, refreshNotesActions]() {
      notesEditor_->clear();
      notesPath_ = makeNotesPath();
      refreshNotesActions();
      updateNotesPreview();
      statusBar()->showMessage("New notes created", 3000);
    };

    connect(copyButton, &QPushButton::clicked, this, [copyNotes]() {
      copyNotes();
    });

    connect(openButton, &QPushButton::clicked, this, [openNotes]() { openNotes(); });

    connect(saveButton, &QPushButton::clicked, this, [saveNotes]() { saveNotes(); });

    connect(saveAsButton, &QPushButton::clicked, this, [saveNotesAs]() { saveNotesAs(); });

    connect(newButton, &QPushButton::clicked, this, [newNotes]() { newNotes(); });

    connect(clearNotesOutputButton, &QPushButton::clicked, this, [this]() {
      notesRunOutput_.clear();
      for (auto &block : notesCodeBlocks_) {
        block.output.clear();
      }
      updateNotesPreview();
      statusBar()->showMessage("Notes output cleared", 3000);
    });

    if (notesSwitchButton_) {
      connect(notesSwitchButton_, &QPushButton::clicked, this,
              [this, makeNotesPath, refreshNotesActions]() {
                const bool isPython = isPythonNotesFile(notesPath_);
                notesEditor_->clear();
                notesPath_ = isPython ? makeNotesPath() : makePythonNotesPath();
                refreshNotesActions();
                updateNotesPreview();
                statusBar()->showMessage(
                    isPython ? "Switched to Markdown notes" : "Switched to Python notes", 3000);
              });
    }

    if (notesRunButton_) {
      connect(notesRunButton_, &QPushButton::clicked, this, [this]() {
        const QString text = notesEditor_->toPlainText();
        const auto blocks = extractPythonBlocks(text);
        QString code;
        if (!blocks.empty()) {
          for (size_t idx = 0; idx < blocks.size(); ++idx) {
            if (idx != 0) {
              code += "\n\n";
            }
            code += blocks[idx];
          }
        } else if (isPythonNotesFile(notesPath_)) {
          code = text;
        }
        if (code.trimmed().isEmpty()) {
          return;
        }

        const PythonRunResult runResult = runPythonSnippet(code);
        const bool success = runResult.started && runResult.exitCode == 0;
        if (output_) {
          output_->setPlainText(success ? "Python: Success" : "Python: Fail");
        }
        statusBar()->showMessage(
            success ? "Python run succeeded" : "Python run failed", 3000);
        if (!runResult.started) {
          notesRunOutput_ = runResult.errorMessage;
        } else {
          const QString combined =
              (runResult.stdoutText + runResult.stderrText).trimmed();
          notesRunOutput_ = combined.isEmpty() ? "No output." : combined;
        }
        for (auto &block : notesCodeBlocks_) {
          block.output.clear();
        }
        updateNotesPreview();
      });
    }

    refreshNotesActions();

    if (notesNewAction_) {
      connect(notesNewAction_, &QAction::triggered, this, [newNotes]() {
        newNotes();
      });
    }
    if (notesOpenAction_) {
      connect(notesOpenAction_, &QAction::triggered, this, [openNotes]() {
        openNotes();
      });
    }
    if (notesSaveAction_) {
      connect(notesSaveAction_, &QAction::triggered, this, [saveNotes]() {
        saveNotes();
      });
    }
    if (notesSaveAsAction_) {
      connect(notesSaveAsAction_, &QAction::triggered, this, [saveNotesAs]() {
        saveNotesAs();
      });
    }
    if (notesCopyAction_) {
      connect(notesCopyAction_, &QAction::triggered, this, [copyNotes]() {
        copyNotes();
      });
    }
  }
  registerTab("Notes", notesTab);

  for (const auto &entry : tabEntries_) {
    splitLeftSelect_->addItem(entry.title);
    splitRightSelect_->addItem(entry.title);
  }
  if (!tabEntries_.empty()) {
    splitLeftSelect_->setCurrentIndex(0);
    splitRightSelect_->setCurrentIndex(tabEntries_.size() > 1 ? 1 : 0);
  }

  output_ = new QPlainTextEdit();
  output_->setReadOnly(true);
  output_->setPlaceholderText("Output appears here");
  output_->setFont(devMonoFont());

  auto *outputGroup = new QVBoxLayout();
  auto *outputHeader = new QHBoxLayout();
  auto *outputLabel = new QLabel("Output");
  auto *copyButton = new QPushButton("Copy");
  auto *moreButton = new QToolButton();
  auto *moreMenu = new QMenu(moreButton);
  auto *copyJson = moreMenu->addAction("Copy as JSON");
  auto *copyYaml = moreMenu->addAction("Copy as YAML");
  auto *copyXml = moreMenu->addAction("Copy as XML");
  moreButton->setText("...");
  moreButton->setMenu(moreMenu);
  moreButton->setPopupMode(QToolButton::InstantPopup);
  outputHeader->addWidget(outputLabel);
  outputHeader->addStretch();
  outputHeader->addWidget(copyButton);
  outputHeader->addWidget(moreButton);
  outputGroup->addLayout(outputHeader);
  outputGroup->addWidget(output_);

  auto *outputContainer = new QWidget(central);
  outputContainer->setLayout(outputGroup);

  auto *mainPanel = new QWidget(central);
  auto *mainPanelLayout = new QVBoxLayout(mainPanel);
  mainPanelLayout->setContentsMargins(0, 0, 0, 0);
  mainPanelLayout->addWidget(splitBarContainer_);
  mainPanelLayout->addWidget(mainContentStack_);

  auto *splitter = new QSplitter(Qt::Vertical, central);
  splitter->addWidget(mainPanel);
  splitter->addWidget(outputContainer);
  splitter->setStretchFactor(0, 3);
  splitter->setStretchFactor(1, 1);
  layout->addWidget(splitter);
  setCentralWidget(central);

  statusBar()->showMessage("Ready");

  helpButton_ = new QToolButton(this);
  helpButton_->setText("?");
  helpButton_->setToolTip("Help");
  helpButton_->setAutoRaise(true);
  statusBar()->addWidget(helpButton_);
  updateHelpTooltip(mainTabs_->currentIndex());

  connect(mainTabs_, &QTabWidget::currentChanged, this,
          [this](int index) { updateHelpTooltip(index); });

  connect(splitLeftSelect_, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, [this](int) {
            updateSplitStacks();
            updateHelpTooltip(mainTabs_->currentIndex());
          });

  connect(splitRightSelect_, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, [this](int) { updateSplitStacks(); });

  connect(splitViewAction, &QAction::toggled, this,
          [this](bool enabled) { setSplitViewEnabled(enabled); });

  connect(toggleNotesEditorAction, &QAction::toggled, this, [this](bool enabled) {
    if (!notesEditor_ || !notesSplitter_) {
      return;
    }
    notesEditor_->setVisible(enabled);
    if (enabled) {
      notesSplitter_->setStretchFactor(0, 1);
      notesSplitter_->setStretchFactor(1, 1);
    } else {
      notesSplitter_->setStretchFactor(0, 0);
      notesSplitter_->setStretchFactor(1, 1);
    }
  });

  if (notesTogglePreviewAction_) {
    connect(notesTogglePreviewAction_, &QAction::toggled, this, [this](bool enabled) {
      if (!notesPreview_ || !notesSplitter_) {
        return;
      }
      notesPreview_->setVisible(enabled);
      if (enabled) {
        notesSplitter_->setStretchFactor(0, 1);
        notesSplitter_->setStretchFactor(1, 1);
      } else {
        notesSplitter_->setStretchFactor(0, 1);
        notesSplitter_->setStretchFactor(1, 0);
      }
    });
  }

  connect(clearOutputAction, &QAction::triggered, this, [this]() {
    output_->clear();
    statusBar()->showMessage("Output cleared", 3000);
  });

  connect(clearHistoryAction, &QAction::triggered, this, [this]() {
    if (exprHistory_) {
      exprHistory_->clear();
    }
    statusBar()->showMessage("Expression history cleared", 3000);
  });

  connect(exitAction, &QAction::triggered, this, [this]() { close(); });

  connect(defaultBigIntAction, &QAction::toggled, this, [this](bool checked) {
    bigIntCheck_->setChecked(checked);
  });

  connect(wrapOutputAction, &QAction::toggled, this, [this](bool checked) {
    output_->setLineWrapMode(checked ? QPlainTextEdit::WidgetWidth
                                    : QPlainTextEdit::NoWrap);
  });

  connect(matrixEditorAction, &QAction::toggled, this, [this](bool checked) {
    matrixInputStack_->setCurrentIndex(checked ? 0 : 1);
    statusBar()->showMessage(
        checked ? "Matrix editor enabled" : "Matrix editor disabled", 3000);
  });

  connect(desktopModeAction, &QAction::triggered, this, [this, desktopModeAction, touchModeAction]() {
    desktopModeAction->setChecked(true);
    touchModeAction->setChecked(false);
    statusBar()->showMessage("Mode: Desktop", 3000);
  });

  connect(touchModeAction, &QAction::triggered, this, [this, desktopModeAction, touchModeAction]() {
    touchModeAction->setChecked(true);
    desktopModeAction->setChecked(false);
    statusBar()->showMessage("Mode: Touchscreen", 3000);
  });

  connect(aboutAction, &QAction::triggered, this, [this]() {
    QMessageBox::information(
        this, "About",
        "CLI Calculator\n GitHub: https://github.com/benedek553/cli-calculator\n\n");
  });

  connect(contributeAction, &QAction::triggered, this, []() {
    QDesktopServices::openUrl(QUrl("https://github.com/benedek553/cli-calculator"));
  });

  connect(reportBugAction, &QAction::triggered, this, []() {
    QDesktopServices::openUrl(QUrl("https://github.com/Benedek553/cli-calculator/issues/new?template=bug_report.yml"));
  });

  connect(requestFeatureAction, &QAction::triggered, this, []() {
    QDesktopServices::openUrl(QUrl("https://github.com/Benedek553/cli-calculator/issues/new?template=feature_request.yml"));
  });

  connect(copyButton, &QPushButton::clicked, this, [this]() {
    const QString text = output_->toPlainText();
    QApplication::clipboard()->setText(text);
    statusBar()->showMessage("Copied output", 3000);
  });

  connect(copyJson, &QAction::triggered, this,
          [this]() { copyStructured(OutputFormat::Json, "Copy as JSON"); });
  connect(copyYaml, &QAction::triggered, this,
          [this]() { copyStructured(OutputFormat::Yaml, "Copy as YAML"); });
  connect(copyXml, &QAction::triggered, this,
          [this]() { copyStructured(OutputFormat::Xml, "Copy as XML"); });
}

void MainWindow::showResult(const QString &title, const QString &text, int exitCode) {
  QString message = text.isEmpty() ? QStringLiteral("No output") : text;
  output_->setPlainText(message);
  statusBar()->showMessage(QString("%1 (exit %2)").arg(title).arg(exitCode), 5000);
}

void MainWindow::runAndShow(const QString &title,
                            const std::function<int(OutputFormat)> &action) {
  lastAction_ = action;
  lastTitle_ = title;
  CliRunResult result = runCliAction([&]() { return action(OutputFormat::Text); });
  QString output = QString::fromStdString(result.out + result.err);
  showResult(title, output.trimmed(), result.exitCode);
}

void MainWindow::copyStructured(OutputFormat format, const QString &label) {
  if (!lastAction_) {
    statusBar()->showMessage("No recent output to copy", 3000);
    return;
  }
  CliRunResult result = runCliAction([&]() { return lastAction_(format); });
  const std::string combined = result.out + result.err;
  QApplication::clipboard()->setText(QString::fromStdString(combined));
  statusBar()->showMessage(QString("%1 (%2)").arg(label).arg(lastTitle_), 4000);
}

void MainWindow::showGraphPreview(const QString &title, const QString &path) {
  lastAction_ = nullptr;
  lastTitle_.clear();
  currentGraphPath_ = path;
  QPixmap pixmap(path);
  if (pixmap.isNull()) {
    showGraphError(title, "Failed to load generated graph.");
    return;
  }
  graphPreviewLabel_->setPixmap(pixmap);
  output_->setPlainText(QString("Saved graph to: %1").arg(path));
  statusBar()->showMessage(QString("%1 generated").arg(title), 4000);
}

void MainWindow::showGraphError(const QString &title, const QString &message) {
  lastAction_ = nullptr;
  lastTitle_.clear();
  output_->setPlainText(message.trimmed().isEmpty()
                            ? QString("Failed to generate graph.")
                            : message.trimmed());
  statusBar()->showMessage(QString("%1 failed").arg(title), 4000);
}

void MainWindow::clearGraphPreview() {
  currentGraphPath_.clear();
  graphPreviewLabel_->setText("No graph generated yet.");
  graphPreviewLabel_->setPixmap(QPixmap());
}

QString MainWindow::makeTempGraphPath() const {
  const QString letters = "abcdefghijklmnopqrstuvwxyz0123456789";
  QString random;
  for (int idx = 0; idx < 10; ++idx) {
    int pos = QRandomGenerator::global()->bounded(letters.size());
    random.append(letters[pos]);
  }
  return QDir::tempPath() + "/tmp_" + random + ".png";
}

void MainWindow::updateHelpTooltip(int tabIndex) {
  if (!helpButton_ || !mainTabs_) {
    return;
  }
  QString tabName;
  if (mainContentStack_ && mainContentStack_->currentWidget() == splitViewContainer_) {
    if (splitLeftSelect_) {
      tabName = splitLeftSelect_->currentText();
    }
  } else {
    tabName = mainTabs_->tabText(tabIndex);
  }
  QString message;
  if (tabName == "Expressions") {
    message = "Evaluate expressions and use BigInt for large integers.";
  } else if (tabName == "Numbers") {
    message = "Square roots, divisor lists, and prime factorization.";
  } else if (tabName == "Conversions") {
    message = "Convert between bases or measurement units.";
  } else if (tabName == "Equations") {
    message = "Solve linear and quadratic equations.";
  } else if (tabName == "Matrices") {
    message = "Add, subtract, or multiply matrices (editor optional).";
  } else if (tabName == "Graphs") {
    message = "Create graphs from values or CSV files and save PNGs.";
  } else if (tabName == "Statistics") {
    message = "Compute summary statistics for numeric lists.";
  } else if (tabName == "Variables") {
    message = "List, set, or unset persisted variables.";
  } else if (tabName == "Notes") {
    message = "Write Markdown notes, preview them, and save to a file.";
  } else if (tabName == "Terminal") {
    message = "Run CLI calculator commands or shell commands in-app.";
  } else {
    message = "Use the tabs to access calculator features.";
  }
  helpButton_->setToolTip(message);
}

QString MainWindow::buildNotesHtml(const QString &markdown) {
  std::vector<QString> previousOutputs;
  previousOutputs.reserve(notesCodeBlocks_.size());
  for (const auto &block : notesCodeBlocks_) {
    previousOutputs.push_back(block.output);
  }
  notesCodeBlocks_.clear();
  const QString text = renderMarkdownWithLatexSymbols(markdown);
  const QRegularExpression fenceRe(
      "```\\s*([\\w#+-]*)[^\\n]*\\n([\\s\\S]*?)```");
  QString placeholderText;
  int lastIndex = 0;
  int index = 0;
  QRegularExpressionMatchIterator it = fenceRe.globalMatch(text);
  while (it.hasNext()) {
    QRegularExpressionMatch match = it.next();
    placeholderText += text.mid(lastIndex, match.capturedStart() - lastIndex);
    const QString language = match.captured(1);
    const QString code = match.captured(2);
    QString output;
    if (index < static_cast<int>(previousOutputs.size())) {
      output = previousOutputs[index];
    }
    notesCodeBlocks_.push_back({code, language, output});
    placeholderText += QString("[[[CODEBLOCK_%1]]]").arg(index++);
    lastIndex = match.capturedEnd();
  }
  placeholderText += text.mid(lastIndex);

  QTextDocument doc;
  doc.setMarkdown(placeholderText);
  QString html = doc.toHtml();
  for (int i = 0; i < static_cast<int>(notesCodeBlocks_.size()); ++i) {
    const QString placeholder = QString("[[[CODEBLOCK_%1]]]").arg(i);
    const QString escaped = QRegularExpression::escape(placeholder);
    const QString pattern =
        QString("<p[^>]*>(?:<span[^>]*>)?%1(?:</span>)?</p>").arg(escaped);
    QRegularExpression replaceRe(pattern);
    html.replace(replaceRe,
                 wrapCodeBlockHtml(i, notesCodeBlocks_[i].code,
                                   notesCodeBlocks_[i].language,
                                   notesCodeBlocks_[i].output));
    html.replace(placeholder,
                 wrapCodeBlockHtml(i, notesCodeBlocks_[i].code,
                                   notesCodeBlocks_[i].language,
                                   notesCodeBlocks_[i].output));
  }
  return html;
}

void MainWindow::updateNotesPreview() {
  if (!notesPreview_) {
    return;
  }
  if (isPythonNotesFile(notesPath_)) {
    const QString output = notesRunOutput_.toHtmlEscaped();
    QString html = "<pre style=\"white-space:pre-wrap;\">";
    html += output;
    html += "</pre>";
    notesPreview_->setHtml(html);
    return;
  }
  QString html = buildNotesHtml(notesEditor_->toPlainText());
  if (!notesRunOutput_.isEmpty()) {
    const QString output = notesRunOutput_.toHtmlEscaped();
    html += "<hr/>";
    html += "<h3>Output</h3>";
    html += "<pre style=\"white-space:pre-wrap;\">";
    html += output;
    html += "</pre>";
  }
  notesPreview_->setHtml(html);
}

bool MainWindow::isNotesPythonMode() const {
  return isPythonNotesFile(notesPath_);
}

bool MainWindow::isNotesCodePosition(int pos) const {
  if (pos < 0) {
    return false;
  }
  if (isNotesPythonMode()) {
    return true;
  }
  for (const auto &range : notesCodeRanges_) {
    if (pos >= range.first && pos < range.second) {
      return true;
    }
  }
  return false;
}

bool MainWindow::isNotesPythonPosition(int pos) const {
  if (pos < 0) {
    return false;
  }
  if (isNotesPythonMode()) {
    return true;
  }
  for (const auto &range : notesPythonRanges_) {
    if (pos >= range.first && pos < range.second) {
      return true;
    }
  }
  return false;
}

void MainWindow::updateNotesRanges() {
  if (!notesEditor_) {
    notesCodeRanges_.clear();
    notesPythonRanges_.clear();
    return;
  }
  const QString text = notesEditor_->toPlainText();
  if (isNotesPythonMode()) {
    notesCodeRanges_.assign(1, {0, text.size()});
    notesPythonRanges_.assign(1, {0, text.size()});
    return;
  }
  notesCodeRanges_ = extractCodeRanges(text);
  notesPythonRanges_ = extractPythonRanges(text);
}

void MainWindow::registerTab(const QString &title, QWidget *widget) {
  mainTabs_->addTab(widget, title);
  tabEntries_.push_back({title, widget});
}

void MainWindow::setSplitViewEnabled(bool enabled) {
  if (!mainContentStack_) {
    return;
  }
  splitBarContainer_->setVisible(enabled);
  if (enabled) {
    int currentIndex = mainTabs_->currentIndex();
    if (currentIndex < 0) {
      currentIndex = 0;
    }
    while (mainTabs_->count() > 0) {
      QWidget *widget = mainTabs_->widget(0);
      mainTabs_->removeTab(0);
      moveTabWidget(widget, tabPool_);
    }
    if (splitLeftSelect_->count() > 0) {
      QSignalBlocker blockLeft(splitLeftSelect_);
      splitLeftSelect_->setCurrentIndex(currentIndex);
    }
    if (splitRightSelect_->count() > 0) {
      int rightIndex = (currentIndex + 1) % splitRightSelect_->count();
      QSignalBlocker blockRight(splitRightSelect_);
      splitRightSelect_->setCurrentIndex(rightIndex);
    }
    mainContentStack_->setCurrentWidget(splitViewContainer_);
    updateSplitStacks();
    updateHelpTooltip(currentIndex);
  } else {
    mainContentStack_->setCurrentWidget(mainTabs_);
    while (mainTabs_->count() > 0) {
      mainTabs_->removeTab(0);
    }
    for (const auto &entry : tabEntries_) {
      if (!entry.widget) {
        continue;
      }
      if (auto stack = qobject_cast<QStackedWidget *>(entry.widget->parentWidget())) {
        stack->removeWidget(entry.widget);
      }
      entry.widget->setParent(mainTabs_);
      mainTabs_->addTab(entry.widget, entry.title);
    }
    updateHelpTooltip(mainTabs_->currentIndex());
  }
}

void MainWindow::updateSplitStacks() {
  if (!mainContentStack_ ||
      mainContentStack_->currentWidget() != splitViewContainer_) {
    return;
  }
  const int count = splitLeftSelect_->count();
  if (count == 0) {
    return;
  }
  int leftIndex = splitLeftSelect_->currentIndex();
  int rightIndex = splitRightSelect_->currentIndex();
  if (leftIndex == rightIndex && count > 1) {
    QSignalBlocker blockRight(splitRightSelect_);
    rightIndex = (rightIndex + 1) % count;
    splitRightSelect_->setCurrentIndex(rightIndex);
  }
  if (leftIndex >= 0 && leftIndex < static_cast<int>(tabEntries_.size())) {
    moveTabWidget(tabEntries_[leftIndex].widget, splitLeftStack_);
  }
  if (rightIndex >= 0 && rightIndex < static_cast<int>(tabEntries_.size())) {
    moveTabWidget(tabEntries_[rightIndex].widget, splitRightStack_);
  }
}

void MainWindow::moveTabWidget(QWidget *widget, QStackedWidget *target) {
  if (!widget || !target) {
    return;
  }
  if (auto stack = qobject_cast<QStackedWidget *>(widget->parentWidget())) {
    stack->removeWidget(widget);
  }
  widget->setParent(target);
  if (target->indexOf(widget) == -1) {
    target->addWidget(widget);
  }
  target->setCurrentWidget(widget);
}
