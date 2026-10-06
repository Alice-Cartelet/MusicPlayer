#include "lyricseditdialog.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QCryptographicHash>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QFontDatabase>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPainter>
#include <QMessageBox>
#include <QMediaPlayer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSignalBlocker>
#include <QSplitter>
#include <QSpinBox>
#include <QStyledItemDelegate>
#include <QTextDocument>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>
#include <QVBoxLayout>
#ifdef Q_OS_WIN
#include <windows.h>
#include <bcrypt.h>
#endif

static QByteArray neteaseLyricPostBody(const QString &id)
{
#ifdef Q_OS_WIN
    const QByteArray path = "/api/song/lyric/v1";
    const QByteArray params = QJsonDocument(QJsonObject{
        {QStringLiteral("id"), id.toLongLong()},
        {QStringLiteral("lv"), QStringLiteral("-1")},
        {QStringLiteral("tv"), QStringLiteral("-1")},
        {QStringLiteral("rv"), QStringLiteral("-1")},
        {QStringLiteral("yv"), QStringLiteral("-1")},
        {QStringLiteral("e_r"), false}
    }).toJson(QJsonDocument::Compact);
    const QByteArray sign = QCryptographicHash::hash(
        "nobody" + path + "use" + params + "md5forencrypt", QCryptographicHash::Md5).toHex();
    const QByteArray plain = path + "-36cd479b6b5-" + params + "-36cd479b6b5-" + sign;
    const QByteArray key = "e82ckenh8dichen8";
    BCRYPT_ALG_HANDLE algorithm = nullptr;
    BCRYPT_KEY_HANDLE cipherKey = nullptr;
    QByteArray encrypted;
    if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_AES_ALGORITHM, nullptr, 0) >= 0) {
        if (BCryptSetProperty(algorithm, BCRYPT_CHAINING_MODE,
                reinterpret_cast<PUCHAR>(const_cast<wchar_t *>(BCRYPT_CHAIN_MODE_ECB)),
                sizeof(BCRYPT_CHAIN_MODE_ECB), 0) >= 0
            && BCryptGenerateSymmetricKey(algorithm, &cipherKey, nullptr, 0,
                reinterpret_cast<PUCHAR>(const_cast<char *>(key.constData())), ULONG(key.size()), 0) >= 0) {
            ULONG length = 0;
            if (BCryptEncrypt(cipherKey,
                    reinterpret_cast<PUCHAR>(const_cast<char *>(plain.constData())), ULONG(plain.size()),
                    nullptr, nullptr, 0, nullptr, 0, &length, BCRYPT_BLOCK_PADDING) >= 0) {
                encrypted.resize(length);
                if (BCryptEncrypt(cipherKey,
                        reinterpret_cast<PUCHAR>(const_cast<char *>(plain.constData())), ULONG(plain.size()),
                        nullptr, nullptr, 0, reinterpret_cast<PUCHAR>(encrypted.data()), length,
                        &length, BCRYPT_BLOCK_PADDING) >= 0) encrypted.resize(length);
                else encrypted.clear();
            }
        }
    }
    if (cipherKey) BCryptDestroyKey(cipherKey);
    if (algorithm) BCryptCloseAlgorithmProvider(algorithm, 0);
    return encrypted.isEmpty() ? QByteArray() : "params=" + encrypted.toHex().toUpper();
#else
    Q_UNUSED(id)
    return {};
#endif
}

static QJsonObject qqComm()
{
    return QJsonObject{
        {QStringLiteral("ct"), 11},
        {QStringLiteral("cv"), QStringLiteral("1003006")},
        {QStringLiteral("v"), QStringLiteral("1003006")},
        {QStringLiteral("os_ver"), QStringLiteral("15")},
        {QStringLiteral("phonetype"), QStringLiteral("24122RKC7C")},
        {QStringLiteral("rom"), QStringLiteral("Redmi/miro/miro:15/AE3A.240806.005/OS2.0.105.0.VOMCNXM:user/release-keys")},
        {QStringLiteral("tmeAppID"), QStringLiteral("qqmusiclight")},
        {QStringLiteral("nettype"), QStringLiteral("NETWORK_WIFI")},
        {QStringLiteral("udid"), QStringLiteral("0")}
    };
}

static QByteArray qqRequest(const QJsonObject &comm, const QString &method,
                            const QString &module, const QJsonObject &param)
{
    return QJsonDocument(QJsonObject{
        {QStringLiteral("comm"), comm},
        {QStringLiteral("request"), QJsonObject{
            {QStringLiteral("method"), method},
            {QStringLiteral("module"), module},
            {QStringLiteral("param"), param}
        }}
    }).toJson(QJsonDocument::Compact);
}

static QString sourceLabel(const QString &source)
{
    if (source == QStringLiteral("qq")) return QStringLiteral("QQ 音乐");
    if (source == QStringLiteral("netease")) return QStringLiteral("网易云音乐");
    if (source == QStringLiteral("kugou")) return QStringLiteral("酷狗音乐");
    return QStringLiteral("LRCLIB");
}

static QString sourceBadge(const QString &source)
{
    if (source == QStringLiteral("qq")) return QStringLiteral("QQ");
    if (source == QStringLiteral("netease")) return QStringLiteral("网易云");
    if (source == QStringLiteral("kugou")) return QStringLiteral("酷狗");
    return QStringLiteral("LRCLIB");
}

static QFont multilingualFont(qreal pointSize = -1.0)
{
    QFont font = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
    QStringList families = font.families();
    const QStringList fallbacks = {
        QStringLiteral("Segoe UI"),
        QStringLiteral("Nirmala UI"),
        QStringLiteral("Leelawadee UI"),
        QStringLiteral("Noto Sans Devanagari"),
        QStringLiteral("Noto Sans"),
        QStringLiteral("Microsoft YaHei UI"),
        QStringLiteral("Arial Unicode MS")
    };
    for (const QString &family : fallbacks) {
        if (!families.contains(family, Qt::CaseInsensitive))
            families.append(family);
    }
    font.setFamilies(families);
    if (pointSize > 0.0)
        font.setPointSizeF(pointSize);
    font.setStyleStrategy(QFont::PreferAntialias);
    return font;
}

static QString normalizedName(QString value)
{
    value = value.normalized(QString::NormalizationForm_C).toCaseFolded();
    value.remove(QRegularExpression(QStringLiteral(R"([\s\p{P}\p{S}]+)")));
    return value;
}

static QString baseTitle(QString value)
{
    value.remove(QRegularExpression(QStringLiteral(R"(\s*[（(【\[].*?[）)】\]])")));
    return normalizedName(value);
}

static QString durationText(qint64 ms)
{
    if (ms <= 0) return QStringLiteral("时长未知");
    const qint64 seconds = ms / 1000;
    return QStringLiteral("%1:%2").arg(seconds / 60)
        .arg(seconds % 60, 2, 10, QChar('0'));
}

static int lyricLineCount(const QString &lyrics)
{
    int count = 0;
    for (const QString &line : lyrics.split(QLatin1Char('\n')))
        if (!line.trimmed().isEmpty()) ++count;
    return count;
}

class LyricsResultItem final : public QListWidgetItem
{
public:
    bool operator<(const QListWidgetItem &other) const override
    {
        const int score = data(Qt::UserRole + 4).toInt();
        const int otherScore = other.data(Qt::UserRole + 4).toInt();
        return score == otherScore
            ? data(Qt::UserRole + 3).toInt() < other.data(Qt::UserRole + 3).toInt()
            : score > otherScore;
    }
};

class LyricsResultDelegate final : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    QSize sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const override
    {
        return QSize(0, 55);
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        const QString source = index.data(Qt::UserRole).toString();
        const QString title = index.data(Qt::UserRole + 1).toString();
        const QString detail = index.data(Qt::UserRole + 2).toString();
        const QRect card = option.rect.adjusted(2, 1, -2, -1);
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        if (option.state & QStyle::State_Selected)
            painter->setBrush(QColor(QStringLiteral("#eee9fa")));
        else if (option.state & QStyle::State_MouseOver)
            painter->setBrush(QColor(QStringLiteral("#f7f4fc")));
        else
            painter->setBrush(Qt::NoBrush);
        painter->setPen(Qt::NoPen);
        painter->drawRoundedRect(card, 7, 7);

        const QString badge = sourceBadge(source);
        QFont small = option.font;
        small.setPointSizeF(qMax(8.0, small.pointSizeF() - 1.0));
        QFontMetrics smallMetrics(small);
        const int badgeWidth = smallMetrics.horizontalAdvance(badge) + 14;
        const QRect badgeRect(card.right() - badgeWidth - 7, card.top() + 7, badgeWidth, 20);
        painter->setBrush(QColor(QStringLiteral("#f3f0fa")));
        painter->drawRoundedRect(badgeRect, 4, 4);
        const QColor sourceColor = source == QStringLiteral("netease") ? QColor(QStringLiteral("#cb5553"))
            : source == QStringLiteral("qq") ? QColor(QStringLiteral("#318e6d"))
            : source == QStringLiteral("kugou") ? QColor(QStringLiteral("#377eb2"))
            : QColor(QStringLiteral("#795eb2"));
        painter->setPen(sourceColor);
        painter->setFont(small);
        painter->drawText(badgeRect, Qt::AlignCenter, badge);

        QFont bold = option.font;
        bold.setBold(true);
        painter->setFont(bold);
        painter->setPen(QColor(QStringLiteral("#292441")));
        const QRect titleRect(card.left() + 8, card.top() + 6,
                              qMax(0, badgeRect.left() - card.left() - 17), 23);
        painter->drawText(titleRect, Qt::AlignLeft | Qt::AlignVCenter,
                          QFontMetrics(bold).elidedText(title, Qt::ElideRight, titleRect.width()));
        painter->setFont(small);
        painter->setPen(QColor(QStringLiteral("#89809f")));
        const QRect detailRect(card.left() + 8, card.top() + 30, qMax(0, card.width() - 16), 18);
        painter->drawText(detailRect, Qt::AlignLeft | Qt::AlignVCenter,
                          smallMetrics.elidedText(detail, Qt::ElideRight, detailRect.width()));
        painter->restore();
    }
};

LyricsEditDialog::LyricsEditDialog(const TrackItem &track,
                                   const QString &lyricPath, const QString &newLyricPath,
                                   QWidget *parent)
    : QDialog(parent), m_track(track), m_lyricPath(lyricPath), m_newLyricPath(newLyricPath)
{
    setWindowTitle(QStringLiteral("查找与编辑歌词 · %1").arg(track.title));
    resize(880, 560);
    setMinimumSize(720, 450);
    setFont(multilingualFont());
    setStyleSheet(R"(
        QDialog { background: #f5f5f9; color: #252438; }
        QFrame#card { background: #ffffff; border: 1px solid #e5e5ee; border-radius: 10px; }
        QFrame#settings { background: #f7f6fb; border: 1px solid #ece9f3; border-radius: 8px; }
        QLabel#title { color: #262143; font-size: 17px; font-weight: 700; }
        QLabel#sectionTitle { color: #292441; font-size: 12px; font-weight: 700; }
        QLabel#muted, QLabel#credit { color: #7b7690; font-size: 10px; }
        QLabel#trackTitle { color: #614a9a; font-size: 11px; font-weight: 600; }
        QLabel#status { color: #5e5080; font-size: 10px; }
        QLabel#credit a { color: #7251b5; }
        QLineEdit, QSpinBox, QDoubleSpinBox {
            background: white; color: #27233e; border: 1px solid #dcd8e9;
            border-radius: 7px; padding: 5px 8px; min-height: 20px;
            selection-background-color: #a78be2;
        }
        QLineEdit:focus, QSpinBox:focus, QDoubleSpinBox:focus { border-color: #9273d1; }
        QSpinBox::up-button, QSpinBox::down-button,
        QDoubleSpinBox::up-button, QDoubleSpinBox::down-button { width: 0px; border: none; }
        QListWidget { background: transparent; border: none; outline: none; }
        QListWidget::item { border-radius: 7px; margin: 1px 0px; }
        QListWidget::item:selected { background: #eee9fa; }
        QListWidget::item:hover { background: #f7f4fc; }
        QPlainTextEdit { background: #fcfbfe; color: #302b45; border: 1px solid #e7e3ef;
                         border-radius: 8px; padding: 8px; selection-background-color: #d7c8f4; }
        QPushButton { background: white; color: #594b7d; border: 1px solid #ded9eb;
                      border-radius: 7px; padding: 6px 12px; font-weight: 600; }
        QPushButton:hover { background: #f1ecfb; border-color: #b9a9dd; }
        QPushButton:disabled { color: #aaa5ba; background: #f7f6fa; }
        QPushButton#primary { background: #7351bb; color: white; border-color: #7351bb; }
        QPushButton#primary:hover { background: #6542ae; }
        QPushButton#segment { border-radius: 0px; border-color: #dcd8e9; padding: 5px 10px; }
        QPushButton#segment:checked { background: #7351bb; color: white; border-color: #7351bb; }
        QPushButton#segment:disabled { background: #f4f2f7; color: #aba6b6; }
        QCheckBox { spacing: 5px; }
        QSplitter::handle { background: transparent; width: 7px; }
    )");

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 10, 12, 8);
    layout->setSpacing(8);

    auto *titleRow = new QHBoxLayout;
    auto *heading = new QLabel(QStringLiteral("查找与编辑歌词"), this);
    heading->setObjectName(QStringLiteral("title"));
    titleRow->addWidget(heading);
    titleRow->addStretch();
    auto *sourceHint = new QLabel(QStringLiteral("聚合搜索 QQ · 网易云 · 酷狗 · LRCLIB"), this);
    sourceHint->setObjectName(QStringLiteral("muted"));
    titleRow->addWidget(sourceHint);
    layout->addLayout(titleRow);
    auto *searchRow = new QHBoxLayout;
    searchRow->setSpacing(7);
    m_query = new QLineEdit(this);
    m_query->setPlaceholderText(QStringLiteral("搜索歌曲名、歌手或“歌手 歌曲名”"));
    m_query->setText((track.artist.isEmpty() || track.artist == QStringLiteral("未知艺术家"))
                         ? track.title : track.artist + QLatin1Char(' ') + track.title);
    m_searchButton = new QPushButton(QStringLiteral("搜索歌词"), this);
    m_searchButton->setObjectName(QStringLiteral("primary"));
    searchRow->addWidget(m_query, 1);
    searchRow->addWidget(m_searchButton);
    layout->addLayout(searchRow);

    auto *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setChildrenCollapsible(false);
    auto *resultsPane = new QFrame(splitter);
    resultsPane->setObjectName(QStringLiteral("card"));
    auto *resultsLayout = new QVBoxLayout(resultsPane);
    resultsLayout->setContentsMargins(10, 9, 10, 7);
    resultsLayout->setSpacing(6);
    auto *resultsTitle = new QLabel(QStringLiteral("搜索结果 · 按匹配度排序"), resultsPane);
    resultsTitle->setObjectName(QStringLiteral("sectionTitle"));
    resultsLayout->addWidget(resultsTitle);
    m_resultsList = new QListWidget(resultsPane);
    m_resultsList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_resultsList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_resultsList->setMouseTracking(true);
    m_resultsList->setItemDelegate(new LyricsResultDelegate(m_resultsList));
    resultsLayout->addWidget(m_resultsList, 1);
    splitter->addWidget(resultsPane);

    auto *editorPane = new QFrame(splitter);
    editorPane->setObjectName(QStringLiteral("card"));
    auto *editorLayout = new QVBoxLayout(editorPane);
    editorLayout->setContentsMargins(10, 9, 10, 10);
    editorLayout->setSpacing(6);
    auto *editorHeading = new QHBoxLayout;
    auto *editorTitle = new QLabel(QStringLiteral("歌词编辑"), editorPane);
    editorTitle->setObjectName(QStringLiteral("sectionTitle"));
    editorHeading->addWidget(editorTitle);
    editorHeading->addStretch();
    m_compareButton = new QPushButton(QStringLiteral("对比原歌词"), editorPane);
    m_compareButton->setEnabled(false);
    m_compareButton->setToolTip(QStringLiteral("选择在线歌词后，对照打开时的本地歌词"));
    editorHeading->addWidget(m_compareButton);
    m_capabilityLabel = new QLabel(QStringLiteral("本地歌词"), editorPane);
    m_capabilityLabel->setObjectName(QStringLiteral("muted"));
    editorHeading->addWidget(m_capabilityLabel);
    editorLayout->addLayout(editorHeading);
    m_trackLabel = new QLabel(QStringLiteral("选择左侧歌曲，或直接编辑本地歌词"), editorPane);
    m_trackLabel->setObjectName(QStringLiteral("trackTitle"));
    m_trackLabel->setWordWrap(true);
    editorLayout->addWidget(m_trackLabel);

    auto *settings = new QFrame(editorPane);
    settings->setObjectName(QStringLiteral("settings"));
    auto *options = new QGridLayout(settings);
    options->setContentsMargins(8, 7, 8, 7);
    options->setHorizontalSpacing(6);
    options->setVerticalSpacing(5);
    auto *formatLabel = new QLabel(QStringLiteral("歌词样式"), settings);
    formatLabel->setObjectName(QStringLiteral("muted"));
    m_formatGroup = new QButtonGroup(this);
    m_lineButton = new QPushButton(QStringLiteral("逐句"), settings);
    m_wordButton = new QPushButton(QStringLiteral("逐字"), settings);
    for (QPushButton *button : {m_lineButton, m_wordButton}) {
        button->setObjectName(QStringLiteral("segment"));
        button->setCheckable(true);
    }
    m_formatGroup->addButton(m_lineButton, 0);
    m_formatGroup->addButton(m_wordButton, 1);
    m_lineButton->setChecked(true);
    auto *formatButtons = new QHBoxLayout;
    formatButtons->setSpacing(3);
    formatButtons->addWidget(m_lineButton);
    formatButtons->addWidget(m_wordButton);
    m_translation = new QCheckBox(QStringLiteral("包含翻译"), settings);
    m_offset = new QSpinBox(settings);
    m_offset->setRange(-60000, 60000);
    m_offset->setSingleStep(100);
    m_offset->setSuffix(QStringLiteral(" ms"));
    m_offset->setToolTip(QStringLiteral("正数使歌词延后，负数使歌词提前"));
    m_speed = new QDoubleSpinBox(settings);
    m_speed->setRange(0.50, 2.00);
    m_speed->setSingleStep(0.01);
    m_speed->setDecimals(2);
    m_speed->setValue(1.00);
    m_speed->setSuffix(QStringLiteral(" ×"));
    m_speed->setToolTip(QStringLiteral("倍率大于 1 时，歌词时间轴会压缩，歌词推进更快"));
    auto *applyButton = new QPushButton(QStringLiteral("应用"), settings);
    options->addWidget(formatLabel, 0, 0);
    options->addLayout(formatButtons, 0, 1, 1, 2);
    options->addWidget(m_translation, 0, 3);
    options->addWidget(applyButton, 0, 4);
    auto *offsetLabel = new QLabel(QStringLiteral("时间偏移"), settings);
    offsetLabel->setObjectName(QStringLiteral("muted"));
    options->addWidget(offsetLabel, 1, 0);
    options->addWidget(m_offset, 1, 1);
    auto *speedLabel = new QLabel(QStringLiteral("速度倍率"), settings);
    speedLabel->setObjectName(QStringLiteral("muted"));
    options->addWidget(speedLabel, 1, 2);
    options->addWidget(m_speed, 1, 3);
    options->setColumnStretch(4, 1);
    editorLayout->addWidget(settings);

    m_editor = new QPlainTextEdit(editorPane);
    m_editor->setPlaceholderText(QStringLiteral("选择歌曲载入歌词，或直接在这里编辑 LRC 歌词。"));
    m_editor->setLineWrapMode(QPlainTextEdit::NoWrap);
    QFont editorFont = multilingualFont(10.0);
    m_editor->setFont(editorFont);
    editorLayout->addWidget(m_editor, 1);
    splitter->addWidget(editorPane);
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 3);
    splitter->setSizes({305, 555});
    layout->addWidget(splitter, 1);

    auto *bottom = new QFrame(this);
    bottom->setObjectName(QStringLiteral("card"));
    auto *bottomLayout = new QHBoxLayout(bottom);
    bottomLayout->setContentsMargins(10, 7, 8, 7);
    auto *statusColumn = new QVBoxLayout;
    statusColumn->setSpacing(2);
    m_status = new QLabel(QStringLiteral("选择搜索结果可载入歌词，也可以直接编辑本地歌词。"), bottom);
    m_status->setObjectName(QStringLiteral("status"));
    m_status->setWordWrap(true);
    statusColumn->addWidget(m_status);
    m_fileLabel = new QLabel(bottom);
    m_fileLabel->setObjectName(QStringLiteral("muted"));
    m_fileLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    statusColumn->addWidget(m_fileLabel);
    bottomLayout->addLayout(statusColumn, 1);
    auto *saveButton = new QPushButton(QStringLiteral("保存歌词"), bottom);
    saveButton->setObjectName(QStringLiteral("primary"));
    auto *closeButton = new QPushButton(QStringLiteral("关闭"), bottom);
    bottomLayout->addWidget(closeButton);
    bottomLayout->addWidget(saveButton);
    layout->addWidget(bottom);

    auto *credit = new QLabel(this);
    credit->setObjectName(QStringLiteral("credit"));
    credit->setText(QStringLiteral("本功能基于开源工具 <a href=\"https://github.com/chenmozhijin/LDDC\">LDDC</a>开发"));
    credit->setOpenExternalLinks(true);
    layout->addWidget(credit);

    m_network = new QNetworkAccessManager(this);
    connect(m_searchButton, &QPushButton::clicked, this, &LyricsEditDialog::search);
    connect(m_query, &QLineEdit::returnPressed, this, &LyricsEditDialog::search);
    connect(m_resultsList, &QListWidget::currentRowChanged, this, &LyricsEditDialog::loadLyrics);
    auto settingsChanged = [this] {
        m_settingsPending = true;
        m_status->setText(QStringLiteral("设置已更改；点击“应用”更新歌词，保存时也会应用。"));
    };
    connect(m_formatGroup, &QButtonGroup::idClicked, this, settingsChanged);
    connect(m_translation, &QCheckBox::toggled, this, settingsChanged);
    connect(m_offset, &QSpinBox::valueChanged, this, settingsChanged);
    connect(m_speed, &QDoubleSpinBox::valueChanged, this, settingsChanged);
    connect(applyButton, &QPushButton::clicked, this, [this] { updatePreview(); });
    connect(m_compareButton, &QPushButton::clicked, this, &LyricsEditDialog::compareLyrics);
    connect(saveButton, &QPushButton::clicked, this, &LyricsEditDialog::saveLyrics);
    connect(closeButton, &QPushButton::clicked, this, &LyricsEditDialog::reject);

    if (!m_lyricPath.isEmpty()) {
        QFile file(m_lyricPath);
        if (file.open(QIODevice::ReadOnly)) {
            m_editor->setPlainText(QString::fromUtf8(file.readAll()));
            m_editor->document()->setModified(false);
            m_content.lines = m_editor->toPlainText();
            m_originalLyrics = m_content.lines;
            m_lastRendered = m_content.lines;
            m_hasContent = true;
        } else {
            m_status->setText(QStringLiteral("无法读取现有歌词文件：%1").arg(file.errorString()));
        }
    }
    const QString savePath = m_lyricPath.isEmpty() ? m_newLyricPath : m_lyricPath;
    m_fileLabel->setText(QStringLiteral("保存为：%1").arg(QFileInfo(savePath).fileName()));
    m_fileLabel->setToolTip(QDir::toNativeSeparators(savePath));
    if (m_track.duration <= 0 && !m_track.filePath.isEmpty()) {
        m_durationProbe = new QMediaPlayer(this);
        connect(m_durationProbe, &QMediaPlayer::durationChanged, this, [this](qint64 duration) {
            if (duration <= 0) return;
            m_track.duration = duration;
            sortResults();
        });
        m_durationProbe->setSource(QUrl::fromLocalFile(m_track.filePath));
    }
    QTimer::singleShot(0, this, &LyricsEditDialog::search);
}

void LyricsEditDialog::search()
{
    const QString keyword = m_query->text().normalized(QString::NormalizationForm_C).trimmed();
    if (keyword.isEmpty()) {
        m_status->setText(QStringLiteral("请输入搜索关键词。"));
        return;
    }
    ++m_searchGeneration;
    const int generation = m_searchGeneration;
    const auto oldSearches = m_searchReplies;
    m_searchReplies.clear();
    for (QNetworkReply *reply : oldSearches) {
        reply->abort();
        reply->deleteLater();
    }
    if (m_lyricReply) {
        QNetworkReply *reply = m_lyricReply;
        m_lyricReply = nullptr;
        reply->abort();
        reply->deleteLater();
    }
    m_results = QJsonArray();
    m_compareButton->setEnabled(false);
    m_resultsList->blockSignals(true);
    m_resultsList->clear();
    m_resultsList->blockSignals(false);
    m_searchErrors.clear();
    m_pendingSearches = 4;
    m_searchButton->setEnabled(false);
    m_status->setText(QStringLiteral("正在聚合搜索 QQ、网易云、酷狗和 LRCLIB…"));
    for (const QString &source : {QStringLiteral("qq"), QStringLiteral("netease"),
                                  QStringLiteral("kugou"), QStringLiteral("lrclib")}) {
        QUrl url;
        QUrlQuery query;
        QString referer;
        if (source == QStringLiteral("qq")) {
            url = QUrl(QStringLiteral("https://c.y.qq.com/soso/fcgi-bin/client_search_cp"));
            query.addQueryItem(QStringLiteral("format"), QStringLiteral("json"));
            query.addQueryItem(QStringLiteral("p"), QStringLiteral("1"));
            query.addQueryItem(QStringLiteral("n"), QStringLiteral("20"));
            query.addQueryItem(QStringLiteral("w"), keyword);
            referer = QStringLiteral("https://y.qq.com/");
        } else if (source == QStringLiteral("netease")) {
            url = QUrl(QStringLiteral("https://music.163.com/api/search/get"));
            query.addQueryItem(QStringLiteral("type"), QStringLiteral("1"));
            query.addQueryItem(QStringLiteral("limit"), QStringLiteral("20"));
            query.addQueryItem(QStringLiteral("s"), keyword);
            referer = QStringLiteral("https://music.163.com/");
        } else if (source == QStringLiteral("kugou")) {
            url = QUrl(QStringLiteral("https://songsearch.kugou.com/song_search_v2"));
            query.addQueryItem(QStringLiteral("keyword"), keyword);
            query.addQueryItem(QStringLiteral("page"), QStringLiteral("1"));
            query.addQueryItem(QStringLiteral("pagesize"), QStringLiteral("20"));
            query.addQueryItem(QStringLiteral("platform"), QStringLiteral("WebFilter"));
            query.addQueryItem(QStringLiteral("filter"), QStringLiteral("2"));
        } else {
            url = QUrl(QStringLiteral("https://lrclib.net/api/search"));
            query.addQueryItem(QStringLiteral("q"), keyword);
        }
        url.setQuery(query);
        requestJson(url, referer, [this, source, generation](const QJsonDocument &document, const QString &error) {
            if (generation != m_searchGeneration) return;
            if (error.isEmpty()) addSearchResults(source, document);
            else m_searchErrors.append(sourceLabel(source) + QStringLiteral("：") + error);
            --m_pendingSearches;
            if (m_pendingSearches == 0) m_searchButton->setEnabled(true);
            if (m_resultsList->currentRow() < 0)
                m_status->setText(m_pendingSearches
                    ? QStringLiteral("正在搜索，已找到 %1 条结果（还剩 %2 个来源）…")
                        .arg(m_results.size()).arg(m_pendingSearches)
                    : QStringLiteral("搜索完成，找到 %1 条结果%2")
                        .arg(m_results.size())
                        .arg(m_searchErrors.isEmpty() ? QString() : QStringLiteral("；%1").arg(m_searchErrors.join(QStringLiteral("；")))));
        }, true);
    }
}

void LyricsEditDialog::requestJson(const QUrl &url, const QString &referer,
    std::function<void(const QJsonDocument &, const QString &)> done, bool searchRequest,
    const QByteArray &postBody)
{
    if (!searchRequest && m_lyricReply) {
        QNetworkReply *old = m_lyricReply;
        m_lyricReply = nullptr;
        old->abort();
        old->deleteLater();
    }
    QNetworkRequest request(url);
    request.setRawHeader("Accept", "application/json");
    request.setRawHeader("User-Agent", "Mozilla/5.0 (compatible; MusicPlayer/2.0)");
    if (!referer.isEmpty()) request.setRawHeader("Referer", referer.toUtf8());
    request.setTransferTimeout(15000);
    if (!postBody.isEmpty()) {
        if (url.host() == QStringLiteral("u.y.qq.com")) {
            request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
            request.setRawHeader("User-Agent", "okhttp/3.14.9");
            request.setRawHeader("Cookie", "tmeLoginType=-1;");
        } else {
            request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/x-www-form-urlencoded"));
            request.setRawHeader("Origin", "orpheus://orpheus");
            request.setRawHeader("Cookie", "os=pc; appver=3.1.3.203419");
            request.setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; WOW64) NeteaseMusicDesktop/3.1.3.203419");
        }
    }
    QNetworkReply *reply = postBody.isEmpty() ? m_network->get(request) : m_network->post(request, postBody);
    if (searchRequest) m_searchReplies.append(reply);
    else m_lyricReply = reply;
    connect(reply, &QNetworkReply::finished, this, [this, reply, searchRequest, done = std::move(done)]() {
        if (searchRequest) {
            if (!m_searchReplies.removeOne(reply)) return;
        } else {
            if (reply != m_lyricReply) return;
            m_lyricReply = nullptr;
        }
        const QByteArray body = reply->readAll();
        const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QString networkError = reply->errorString();
        const bool failed = reply->error() != QNetworkReply::NoError || httpStatus != 200;
        reply->deleteLater();
        if (failed) {
            done(QJsonDocument(), httpStatus == 429
                ? QStringLiteral("请求过于频繁，请稍后再试") : networkError);
            return;
        }
        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(body, &parseError);
        done(document, parseError.error == QJsonParseError::NoError
            ? QString() : QStringLiteral("服务器返回了无效数据"));
    });
}

void LyricsEditDialog::addSearchResults(const QString &source, const QJsonDocument &document)
{
    QJsonArray songs;
    if (source == QStringLiteral("qq")) {
        songs = document.object().value(QStringLiteral("data")).toObject()
            .value(QStringLiteral("song")).toObject().value(QStringLiteral("list")).toArray();
        for (const QJsonValue &value : songs) {
            const QJsonObject song = value.toObject();
            QStringList artists;
            for (const QJsonValue &singer : song.value(QStringLiteral("singer")).toArray())
                artists.append(singer.toObject().value(QStringLiteral("name")).toString());
            addResult(QJsonObject{{"source", source},
                                  {"title", song.value(QStringLiteral("songname"))},
                                  {"artist", artists.join(QStringLiteral(" / "))},
                                  {"album", song.value(QStringLiteral("albumname"))},
                                  {"mid", song.value(QStringLiteral("songmid"))},
                                  {"id", QString::number(qint64(song.value(QStringLiteral("songid")).toDouble()))},
                                  {"duration", song.value(QStringLiteral("interval")).toInt()},
                                  {"durationMs", song.value(QStringLiteral("interval")).toInt() * 1000}});
        }
    } else if (source == QStringLiteral("netease")) {
        songs = document.object().value(QStringLiteral("result")).toObject()
            .value(QStringLiteral("songs")).toArray();
        for (const QJsonValue &value : songs) {
            const QJsonObject song = value.toObject();
            QStringList artists;
            for (const QJsonValue &singer : song.value(QStringLiteral("artists")).toArray())
                artists.append(singer.toObject().value(QStringLiteral("name")).toString());
            addResult(QJsonObject{{"source", source},
                                  {"title", song.value(QStringLiteral("name"))},
                                  {"artist", artists.join(QStringLiteral(" / "))},
                                  {"album", song.value(QStringLiteral("album")).toObject().value(QStringLiteral("name"))},
                                  {"id", QString::number(qint64(song.value(QStringLiteral("id")).toDouble()))},
                                  {"durationMs", song.value(QStringLiteral("duration")).toInt()}});
        }
    } else if (source == QStringLiteral("kugou")) {
        songs = document.object().value(QStringLiteral("data")).toObject()
            .value(QStringLiteral("lists")).toArray();
        for (const QJsonValue &value : songs) {
            const QJsonObject song = value.toObject();
            addResult(QJsonObject{{"source", source},
                                  {"title", song.value(QStringLiteral("SongName"))},
                                  {"artist", song.value(QStringLiteral("SingerName"))},
                                  {"album", song.value(QStringLiteral("AlbumName"))},
                                  {"id", song.value(QStringLiteral("ID")).toString()},
                                  {"hash", song.value(QStringLiteral("FileHash"))},
                                  {"duration", song.value(QStringLiteral("Duration")).toInt() * 1000},
                                  {"durationMs", song.value(QStringLiteral("Duration")).toInt() * 1000}});
        }
    } else if (document.isArray()) {
        songs = document.array();
        for (const QJsonValue &value : songs) {
            const QJsonObject song = value.toObject();
            addResult(QJsonObject{{"source", source},
                                  {"title", song.value(QStringLiteral("trackName"))},
                                  {"artist", song.value(QStringLiteral("artistName"))},
                                  {"album", song.value(QStringLiteral("albumName"))},
                                  {"durationMs", qRound64(song.value(QStringLiteral("duration")).toDouble() * 1000)},
                                  {"lyrics", !song.value(QStringLiteral("syncedLyrics")).toString().isEmpty()
                                      ? song.value(QStringLiteral("syncedLyrics"))
                                      : song.value(QStringLiteral("plainLyrics"))}});
        }
    }
}

void LyricsEditDialog::addResult(const QJsonObject &result)
{
    if (result.value(QStringLiteral("title")).toString().isEmpty()) return;
    const int index = m_results.size();
    m_results.append(result);
    auto *item = new LyricsResultItem;
    item->setData(Qt::UserRole, result.value(QStringLiteral("source")).toString());
    item->setData(Qt::UserRole + 1, result.value(QStringLiteral("title")).toString());
    item->setData(Qt::UserRole + 3, index);
    {
        QSignalBlocker blocker(m_resultsList);
        m_resultsList->addItem(item);
    }
    sortResults();
}

int LyricsEditDialog::matchScore(const QJsonObject &result, QStringList *reasons) const
{
    int score = 0;
    const QString title = normalizedName(result.value(QStringLiteral("title")).toString());
    const QString localTitle = normalizedName(m_track.title);
    if (!title.isEmpty() && !localTitle.isEmpty()) {
        if (title == localTitle) {
            score += 80;
            if (reasons) reasons->append(QStringLiteral("歌名一致"));
        } else if (baseTitle(result.value(QStringLiteral("title")).toString()) == baseTitle(m_track.title)) {
            score += 58;
            if (reasons) reasons->append(QStringLiteral("歌名主体一致"));
        } else if (title.contains(localTitle) || localTitle.contains(title)) {
            score += 35;
            if (reasons) reasons->append(QStringLiteral("歌名相近"));
        } else {
            score -= 30;
        }
    }
    const QString artist = normalizedName(result.value(QStringLiteral("artist")).toString());
    const QString localArtist = normalizedName(m_track.artist);
    if (!artist.isEmpty() && !localArtist.isEmpty()
        && localArtist != QStringLiteral("未知艺术家")) {
        if (artist == localArtist) {
            score += 45;
            if (reasons) reasons->append(QStringLiteral("演唱者一致"));
        } else if (artist.contains(localArtist) || localArtist.contains(artist)) {
            score += 25;
            if (reasons) reasons->append(QStringLiteral("演唱者部分匹配"));
        } else {
            score -= 12;
        }
    }
    const qint64 remoteDuration = result.value(QStringLiteral("durationMs")).toInteger();
    if (m_track.duration > 0 && remoteDuration > 0) {
        const qint64 difference = qAbs(m_track.duration - remoteDuration);
        if (difference <= 5000) score += 30;
        else if (difference <= 12000) score += 20;
        else if (difference <= 25000) score += 8;
        else if (difference > 45000) score -= 25;
        if (reasons) reasons->append(QStringLiteral("时长相差 %1 秒").arg(qRound64(difference / 1000.0)));
    }
    const QString album = normalizedName(result.value(QStringLiteral("album")).toString());
    if (!album.isEmpty() && !m_track.album.isEmpty() && album == normalizedName(m_track.album)) {
        score += 5;
        if (reasons) reasons->append(QStringLiteral("专辑一致"));
    }
    return score;
}

void LyricsEditDialog::sortResults()
{
    if (!m_resultsList) return;
    QSignalBlocker blocker(m_resultsList);
    for (int row = 0; row < m_resultsList->count(); ++row) {
        QListWidgetItem *item = m_resultsList->item(row);
        const QJsonObject result = m_results.at(item->data(Qt::UserRole + 3).toInt()).toObject();
        QStringList reasons;
        const int score = matchScore(result, &reasons);
        const QString artist = result.value(QStringLiteral("artist")).toString();
        const qint64 remoteDuration = result.value(QStringLiteral("durationMs")).toInteger();
        const QString match = score >= 120 ? QStringLiteral("高度匹配 · ")
            : score >= 80 ? QStringLiteral("较匹配 · ") : QString();
        item->setData(Qt::UserRole + 4, score);
        item->setData(Qt::UserRole + 2, match
            + (artist.isEmpty() ? QStringLiteral("未知歌手") : artist)
            + QStringLiteral(" · ") + durationText(remoteDuration));
        item->setToolTip(QStringLiteral("%1\n%2 · %3\n%4\n匹配依据：%5")
            .arg(result.value(QStringLiteral("title")).toString(), artist,
                 result.value(QStringLiteral("album")).toString(),
                 sourceLabel(result.value(QStringLiteral("source")).toString()),
                 reasons.isEmpty() ? QStringLiteral("暂无可匹配的信息") : reasons.join(QStringLiteral("、"))));
    }
    m_resultsList->sortItems();
}

int LyricsEditDialog::selectedResultIndex() const
{
    const QListWidgetItem *item = m_resultsList->currentItem();
    return item ? item->data(Qt::UserRole + 3).toInt() : -1;
}

bool LyricsEditDialog::confirmReplaceEditor()
{
    if (m_editor->toPlainText() == m_lastRendered) return true;
    return QMessageBox::question(this, QStringLiteral("未保存的修改"),
        QStringLiteral("载入在线歌词会替换当前编辑内容。确定放弃未保存的修改吗？"),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes;
}

void LyricsEditDialog::loadLyrics(int row)
{
    m_compareButton->setEnabled(false);
    if (row < 0 || row >= m_resultsList->count()) return;
    row = m_resultsList->item(row)->data(Qt::UserRole + 3).toInt();
    if (row < 0 || row >= m_results.size()) return;
    const QJsonObject song = m_results.at(row).toObject();
    const QString source = song.value(QStringLiteral("source")).toString();
    if (source == QStringLiteral("lrclib")) {
        LyricsContent content;
        content.lines = song.value(QStringLiteral("lyrics")).toString();
        setContent(row, content);
        return;
    }
    if (source == QStringLiteral("netease")) {
        const QString id = song.value(QStringLiteral("id")).toString();
        const QByteArray body = neteaseLyricPostBody(id);
        if (body.isEmpty()) {
            loadNeteaseLegacy(row, id);
            return;
        }
        m_status->setText(QStringLiteral("正在获取网易云逐字歌词…"));
        requestJson(QUrl(QStringLiteral("https://interface.music.163.com/eapi/song/lyric/v1")),
            QString(), [this, row, id](const QJsonDocument &document, const QString &error) {
                if (row != selectedResultIndex()) return;
                const QJsonObject data = document.object();
                if (!error.isEmpty() || data.value(QStringLiteral("code")).toInt() != 200) {
                    loadNeteaseLegacy(row, id);
                    return;
                }
                const LyricsContent content = LyricsFormat::decodeNetease(
                    data.value(QStringLiteral("yrc")).toObject().value(QStringLiteral("lyric")).toString(),
                    data.value(QStringLiteral("lrc")).toObject().value(QStringLiteral("lyric")).toString(),
                    data.value(QStringLiteral("tlyric")).toObject().value(QStringLiteral("lyric")).toString());
                setContent(row, content);
            }, false, body);
        return;
    }
    if (source == QStringLiteral("qq")) {
        loadQqCloud(row, song);
        return;
    }
    QUrl url;
    QUrlQuery query;
    if (source == QStringLiteral("kugou")) {
        url = QUrl(QStringLiteral("https://lyrics.kugou.com/search"));
        query.addQueryItem(QStringLiteral("ver"), QStringLiteral("1"));
        query.addQueryItem(QStringLiteral("man"), QStringLiteral("yes"));
        query.addQueryItem(QStringLiteral("client"), QStringLiteral("pc"));
        query.addQueryItem(QStringLiteral("keyword"), song.value(QStringLiteral("artist")).toString()
            + QStringLiteral(" - ") + song.value(QStringLiteral("title")).toString());
        query.addQueryItem(QStringLiteral("duration"), QString::number(song.value(QStringLiteral("duration")).toInt()));
        query.addQueryItem(QStringLiteral("hash"), song.value(QStringLiteral("hash")).toString());
    }
    url.setQuery(query);
    m_status->setText(QStringLiteral("正在获取歌词…"));
    requestJson(url, QString(), [this, row](const QJsonDocument &document, const QString &error) {
        if (row != selectedResultIndex()) return;
        if (!error.isEmpty()) {
            m_status->setText(QStringLiteral("获取歌词失败：%1").arg(error));
            return;
        }
        const QJsonObject data = document.object();
            const QJsonArray candidates = data.value(QStringLiteral("candidates")).toArray();
            if (candidates.isEmpty()) {
                m_status->setText(QStringLiteral("酷狗没有找到这首歌的歌词。"));
                return;
            }
            const QJsonObject candidate = candidates.first().toObject();
            QUrl download(QStringLiteral("https://lyrics.kugou.com/download"));
            QUrlQuery params;
            params.addQueryItem(QStringLiteral("ver"), QStringLiteral("1"));
            params.addQueryItem(QStringLiteral("client"), QStringLiteral("pc"));
            params.addQueryItem(QStringLiteral("id"), candidate.value(QStringLiteral("id")).toString());
            params.addQueryItem(QStringLiteral("accesskey"), candidate.value(QStringLiteral("accesskey")).toString());
            params.addQueryItem(QStringLiteral("fmt"), QStringLiteral("krc"));
            params.addQueryItem(QStringLiteral("charset"), QStringLiteral("utf8"));
            download.setQuery(params);
            requestJson(download, QString(), [this, row, candidate](const QJsonDocument &lyricsDoc, const QString &downloadError) {
                if (row != selectedResultIndex()) return;
                if (!downloadError.isEmpty()) {
                    loadKugouLrc(row, candidate);
                    return;
                }
                const QJsonObject payload = lyricsDoc.object();
                const QByteArray decoded = QByteArray::fromBase64(
                    payload.value(QStringLiteral("content")).toString().toLatin1());
                const LyricsContent content = LyricsFormat::decodeKrc(decoded);
                if (content.lines.isEmpty()) {
                    loadKugouLrc(row, candidate);
                    return;
                }
                setContent(row, content);
            });
    });
}

void LyricsEditDialog::loadQqCloud(int row, const QJsonObject &song)
{
    const QString mid = song.value(QStringLiteral("mid")).toString();
    const qint64 id = song.value(QStringLiteral("id")).toString().toLongLong();
    if (id <= 0) {
        loadQqLegacy(row, mid);
        return;
    }
    const QUrl endpoint(QStringLiteral("https://u.y.qq.com/cgi-bin/musicu.fcg"));
    m_status->setText(QStringLiteral("正在获取 QQ 音乐逐字歌词…"));
    requestJson(endpoint, QString(), [this, row, song, mid, endpoint](const QJsonDocument &sessionDoc, const QString &error) {
        if (row != selectedResultIndex()) return;
        const QJsonObject session = sessionDoc.object().value(QStringLiteral("request")).toObject()
            .value(QStringLiteral("data")).toObject().value(QStringLiteral("session")).toObject();
        if (!error.isEmpty() || session.isEmpty()) {
            loadQqLegacy(row, mid);
            return;
        }
        QJsonObject comm = qqComm();
        comm.insert(QStringLiteral("uid"), session.value(QStringLiteral("uid")));
        comm.insert(QStringLiteral("sid"), session.value(QStringLiteral("sid")));
        comm.insert(QStringLiteral("userip"), session.value(QStringLiteral("userip")));
        auto base64 = [](const QString &value) {
            return QString::fromLatin1(value.toUtf8().toBase64());
        };
        const QJsonObject param{
            {QStringLiteral("albumName"), base64(song.value(QStringLiteral("album")).toString())},
            {QStringLiteral("crypt"), 1}, {QStringLiteral("ct"), 19}, {QStringLiteral("cv"), 2111},
            {QStringLiteral("interval"), song.value(QStringLiteral("duration")).toInt()},
            {QStringLiteral("lrc_t"), 0}, {QStringLiteral("qrc"), 1}, {QStringLiteral("qrc_t"), 0},
            {QStringLiteral("roma"), 1}, {QStringLiteral("roma_t"), 0},
            {QStringLiteral("singerName"), base64(song.value(QStringLiteral("artist")).toString())},
            {QStringLiteral("songID"), song.value(QStringLiteral("id")).toString().toLongLong()},
            {QStringLiteral("songName"), base64(song.value(QStringLiteral("title")).toString())},
            {QStringLiteral("trans"), 1}, {QStringLiteral("trans_t"), 0}, {QStringLiteral("type"), 0}
        };
        requestJson(endpoint, QString(), [this, row, mid](const QJsonDocument &lyricDoc, const QString &lyricError) {
            if (row != selectedResultIndex()) return;
            const QJsonObject response = lyricDoc.object().value(QStringLiteral("request")).toObject();
            const QJsonObject data = response.value(QStringLiteral("data")).toObject();
            if (!lyricError.isEmpty() || response.value(QStringLiteral("code")).toInt(-1) != 0) {
                loadQqLegacy(row, mid);
                return;
            }
            const LyricsContent content = LyricsFormat::decodeQqCloud(
                data.value(QStringLiteral("lyric")).toString(),
                data.value(QStringLiteral("trans")).toString());
            if (content.lines.trimmed().isEmpty()) {
                loadQqLegacy(row, mid);
                return;
            }
            setContent(row, content);
        }, false, qqRequest(comm, QStringLiteral("GetPlayLyricInfo"),
            QStringLiteral("music.musichallSong.PlayLyricInfo"), param));
    }, false, qqRequest(qqComm(), QStringLiteral("GetSession"),
        QStringLiteral("music.getSession.session"), QJsonObject{
            {QStringLiteral("caller"), 0}, {QStringLiteral("uid"), QStringLiteral("0")},
            {QStringLiteral("vkey"), 0}
        }));
}

void LyricsEditDialog::loadQqLegacy(int row, const QString &mid)
{
    QUrl url(QStringLiteral("https://c.y.qq.com/lyric/fcgi-bin/fcg_query_lyric_new.fcg"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("songmid"), mid);
    query.addQueryItem(QStringLiteral("format"), QStringLiteral("json"));
    query.addQueryItem(QStringLiteral("nobase64"), QStringLiteral("1"));
    url.setQuery(query);
    m_status->setText(QStringLiteral("逐字歌词不可用，正在获取 QQ 音乐逐句歌词…"));
    requestJson(url, QStringLiteral("https://y.qq.com/"),
        [this, row](const QJsonDocument &document, const QString &error) {
            if (row != selectedResultIndex()) return;
            if (!error.isEmpty()) {
                m_status->setText(QStringLiteral("获取 QQ 音乐歌词失败：%1").arg(error));
                return;
            }
            LyricsContent content;
            content.lines = document.object().value(QStringLiteral("lyric")).toString();
            content.translation = document.object().value(QStringLiteral("trans")).toString();
            setContent(row, content);
        });
}

void LyricsEditDialog::loadNeteaseLegacy(int row, const QString &id)
{
    QUrl url(QStringLiteral("https://music.163.com/api/song/lyric"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("id"), id);
    query.addQueryItem(QStringLiteral("lv"), QStringLiteral("1"));
    query.addQueryItem(QStringLiteral("tv"), QStringLiteral("1"));
    url.setQuery(query);
    m_status->setText(QStringLiteral("正在获取网易云逐句歌词…"));
    requestJson(url, QStringLiteral("https://music.163.com/"),
        [this, row](const QJsonDocument &document, const QString &error) {
            if (row != selectedResultIndex()) return;
            if (!error.isEmpty()) {
                m_status->setText(QStringLiteral("获取网易云歌词失败：%1").arg(error));
                return;
            }
            const QJsonObject data = document.object();
            const LyricsContent content = LyricsFormat::decodeNetease(QString(),
                data.value(QStringLiteral("lrc")).toObject().value(QStringLiteral("lyric")).toString(),
                data.value(QStringLiteral("tlyric")).toObject().value(QStringLiteral("lyric")).toString());
            setContent(row, content);
        });
}

void LyricsEditDialog::loadKugouLrc(int row, const QJsonObject &candidate)
{
    QUrl download(QStringLiteral("https://lyrics.kugou.com/download"));
    QUrlQuery params;
    params.addQueryItem(QStringLiteral("ver"), QStringLiteral("1"));
    params.addQueryItem(QStringLiteral("client"), QStringLiteral("pc"));
    params.addQueryItem(QStringLiteral("id"), candidate.value(QStringLiteral("id")).toString());
    params.addQueryItem(QStringLiteral("accesskey"), candidate.value(QStringLiteral("accesskey")).toString());
    params.addQueryItem(QStringLiteral("fmt"), QStringLiteral("lrc"));
    params.addQueryItem(QStringLiteral("charset"), QStringLiteral("utf8"));
    download.setQuery(params);
    m_status->setText(QStringLiteral("逐字歌词不可用，正在获取逐句歌词…"));
    requestJson(download, QString(), [this, row](const QJsonDocument &document, const QString &error) {
        if (row != selectedResultIndex()) return;
        if (!error.isEmpty()) {
            m_status->setText(QStringLiteral("获取歌词失败：%1").arg(error));
            return;
        }
        LyricsContent content;
        content.lines = QString::fromUtf8(QByteArray::fromBase64(
            document.object().value(QStringLiteral("content")).toString().toLatin1()));
        setContent(row, content);
    });
}

void LyricsEditDialog::setContent(int row, const LyricsContent &content)
{
    if (row != selectedResultIndex()) return;
    if (content.lines.trimmed().isEmpty()) {
        m_status->setText(QStringLiteral("此结果没有可用歌词，请选择其他结果。"));
        return;
    }
    if (!confirmReplaceEditor()) {
        m_resultsList->blockSignals(true);
        m_resultsList->setCurrentRow(-1);
        m_resultsList->blockSignals(false);
        return;
    }
    m_content = content;
    m_hasContent = true;
    const QJsonObject selected = m_results.at(row).toObject();
    m_trackLabel->setText(selected.value(QStringLiteral("title")).toString()
        + QStringLiteral("  ·  ") + selected.value(QStringLiteral("artist")).toString());
    m_capabilityLabel->setText(content.words.isEmpty()
        ? QStringLiteral("逐句可用") : QStringLiteral("逐字 / 逐句可用"));
    m_wordButton->setToolTip(content.words.isEmpty()
        ? QStringLiteral("此结果没有逐字时间数据") : QString());
    bool switchedToLines = false;
    if (m_formatGroup->checkedId() == 1 && content.words.isEmpty()) {
        m_lineButton->setChecked(true);
        switchedToLines = true;
    }
    m_wordButton->setEnabled(!content.words.isEmpty());
    if (!updatePreview(true)) return;
    m_compareButton->setEnabled(true);
    if (switchedToLines) m_status->setText(QStringLiteral("此结果没有逐字时间数据，已切换为逐句歌词。"));
}

void LyricsEditDialog::compareLyrics()
{
    const int index = selectedResultIndex();
    if (index < 0 || index >= m_results.size() || !m_compareButton->isEnabled()) return;
    const QJsonObject result = m_results.at(index).toObject();
    QString renderError;
    QString onlineLyrics = LyricsFormat::render(m_content, m_formatGroup->checkedId() == 1,
        m_translation->isChecked(), m_offset->value(), m_speed->value(), &renderError);
    if (!renderError.isEmpty()) onlineLyrics = m_content.lines;

    QDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("歌词对比 · %1").arg(m_track.title));
    dialog.resize(900, 500);
    dialog.setMinimumSize(700, 380);
    auto *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(7);
    const QString localArtist = m_track.artist.isEmpty() ? QStringLiteral("未知演唱者") : m_track.artist;
    auto *localMeta = new QLabel(QStringLiteral("本地歌曲：%1 · %2 · %3")
        .arg(m_track.title, localArtist, durationText(m_track.duration)), &dialog);
    layout->addWidget(localMeta);
    QStringList reasons;
    matchScore(result, &reasons);
    auto *remoteMeta = new QLabel(QStringLiteral("在线结果：%1 · %2 · %3 · %4")
        .arg(result.value(QStringLiteral("title")).toString(),
             result.value(QStringLiteral("artist")).toString(),
             durationText(result.value(QStringLiteral("durationMs")).toInteger()),
             sourceLabel(result.value(QStringLiteral("source")).toString())), &dialog);
    layout->addWidget(remoteMeta);
    auto *matchLabel = new QLabel(QStringLiteral("匹配依据：%1")
        .arg(reasons.isEmpty() ? QStringLiteral("信息不足，请人工核对") : reasons.join(QStringLiteral("、"))), &dialog);
    matchLabel->setWordWrap(true);
    layout->addWidget(matchLabel);

    auto *splitter = new QSplitter(Qt::Horizontal, &dialog);
    auto addPane = [this, splitter](const QString &heading, const QString &text) {
        auto *pane = new QWidget(splitter);
        auto *paneLayout = new QVBoxLayout(pane);
        paneLayout->setContentsMargins(0, 0, 0, 0);
        auto *label = new QLabel(heading, pane);
        paneLayout->addWidget(label);
        auto *editor = new QPlainTextEdit(pane);
        editor->setReadOnly(true);
        editor->setLineWrapMode(QPlainTextEdit::NoWrap);
        editor->setFont(m_editor->font());
        editor->setPlainText(text);
        paneLayout->addWidget(editor);
        splitter->addWidget(pane);
    };
    addPane(QStringLiteral("原有歌词 · %1 行").arg(lyricLineCount(m_originalLyrics)),
        m_originalLyrics.isEmpty() ? QStringLiteral("（尚无本地歌词）") : m_originalLyrics);
    addPane(QStringLiteral("搜索得到的歌词 · %1 行").arg(lyricLineCount(onlineLyrics)), onlineLyrics);
    splitter->setChildrenCollapsible(false);
    splitter->setSizes({440, 440});
    layout->addWidget(splitter, 1);
    auto *buttons = new QHBoxLayout;
    buttons->addStretch();
    auto *close = new QPushButton(QStringLiteral("关闭对比"), &dialog);
    buttons->addWidget(close);
    layout->addLayout(buttons);
    connect(close, &QPushButton::clicked, &dialog, &QDialog::accept);
    dialog.exec();
}

bool LyricsEditDialog::updatePreview(bool replacing)
{
    if (!m_hasContent) {
        m_status->setText(QStringLiteral("请先选择一条在线结果或打开本地歌词。"));
        return false;
    }
    QString error;
    const QString rendered = LyricsFormat::render(m_content, m_formatGroup->checkedId() == 1,
        m_translation->isChecked(), m_offset->value(), m_speed->value(), &error);
    if (!error.isEmpty()) {
        m_status->setText(error.trimmed());
        return false;
    }
    if (!replacing && m_editor->toPlainText() != m_lastRendered
        && QMessageBox::question(this, QStringLiteral("手动修改的歌词"),
            QStringLiteral("应用设置会重新生成歌词，覆盖手动修改。继续吗？"),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes) return false;
    m_editor->setPlainText(rendered);
    m_editor->document()->setModified(true);
    m_lastRendered = rendered;
    m_settingsPending = false;
    m_status->setText(m_translation->isChecked() && m_content.translation.isEmpty()
        ? QStringLiteral("已载入歌词；此结果没有翻译。")
        : QStringLiteral("歌词设置已应用，可继续编辑并保存。"));
    return true;
}

void LyricsEditDialog::saveLyrics()
{
    if (m_settingsPending && !updatePreview()) return;
    const QString lyrics = m_editor->toPlainText();
    if (lyrics.trimmed().isEmpty()) {
        QMessageBox::information(this, QStringLiteral("歌词为空"), QStringLiteral("请输入或选择歌词后再保存。"));
        return;
    }
    const QString path = m_lyricPath.isEmpty() ? m_newLyricPath : m_lyricPath;
    if (!QDir().mkpath(QFileInfo(path).absolutePath())) {
        QMessageBox::warning(this, QStringLiteral("保存失败"), QStringLiteral("无法创建歌词目录。"));
        return;
    }
    QSaveFile file(path);
    const QByteArray bytes = lyrics.toUtf8();
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit()) {
        QMessageBox::warning(this, QStringLiteral("保存失败"), QStringLiteral("无法保存歌词：%1").arg(file.errorString()));
        return;
    }
    m_lyricPath = path;
    m_saved = true;
    m_editor->document()->setModified(false);
    m_fileLabel->setText(QStringLiteral("保存为：%1").arg(QFileInfo(path).fileName()));
    m_fileLabel->setToolTip(QDir::toNativeSeparators(path));
    static const QRegularExpression timestamp(QStringLiteral(R"(\[\d+:\d+\.\d+\])"));
    m_status->setText(timestamp.match(lyrics).hasMatch()
        ? QStringLiteral("歌词已保存。")
        : QStringLiteral("歌词已保存；请添加时间标签，以便播放时同步显示。"));
}

void LyricsEditDialog::reject()
{
    if (m_editor->document()->isModified() && QMessageBox::question(this,
        QStringLiteral("未保存的修改"), QStringLiteral("当前歌词尚未保存，确定关闭吗？"),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes) return;
    QDialog::reject();
}
