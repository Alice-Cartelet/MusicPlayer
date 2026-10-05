#pragma once

#include <QDialog>
#include <QJsonArray>
#include <QList>
#include <QString>
#include <QStringList>
#include "lyricsformat.h"
#include "trackitem.h"
#include <functional>

class QButtonGroup;
class QCheckBox;
class QDoubleSpinBox;
class QJsonDocument;
class QJsonObject;
class QUrl;
class QSpinBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QNetworkAccessManager;
class QNetworkReply;
class QMediaPlayer;
class QPlainTextEdit;
class QPushButton;

class LyricsEditDialog : public QDialog
{
    Q_OBJECT
public:
    LyricsEditDialog(const TrackItem &track,
                     const QString &lyricPath, const QString &newLyricPath,
                     QWidget *parent = nullptr);
    bool saved() const { return m_saved; }

protected:
    void reject() override;

private:
    void search();
    void loadLyrics(int row);
    void loadQqCloud(int row, const QJsonObject &song);
    void loadQqLegacy(int row, const QString &mid);
    void loadNeteaseLegacy(int row, const QString &id);
    void loadKugouLrc(int row, const QJsonObject &candidate);
    void setContent(int row, const LyricsContent &content);
    bool updatePreview(bool replacing = false);
    void saveLyrics();
    bool confirmReplaceEditor();
    void requestJson(const QUrl &url, const QString &referer,
                     std::function<void(const QJsonDocument &, const QString &)> done,
                     bool searchRequest = false, const QByteArray &postBody = {});
    void addSearchResults(const QString &source, const QJsonDocument &document);
    void addResult(const QJsonObject &result);
    void compareLyrics();
    void sortResults();
    int selectedResultIndex() const;
    int matchScore(const QJsonObject &result, QStringList *reasons = nullptr) const;

    TrackItem m_track;
    QString m_lyricPath;
    QString m_newLyricPath;
    QString m_originalLyrics;
    bool m_saved = false;
    QJsonArray m_results;
    LyricsContent m_content;
    QString m_lastRendered;
    QStringList m_searchErrors;
    int m_searchGeneration = 0;
    int m_pendingSearches = 0;
    bool m_hasContent = false;
    bool m_settingsPending = false;
    QNetworkAccessManager *m_network = nullptr;
    QMediaPlayer *m_durationProbe = nullptr;
    QList<QNetworkReply *> m_searchReplies;
    QNetworkReply *m_lyricReply = nullptr;
    QLineEdit *m_query = nullptr;
    QButtonGroup *m_formatGroup = nullptr;
    QPushButton *m_lineButton = nullptr;
    QPushButton *m_wordButton = nullptr;
    QCheckBox *m_translation = nullptr;
    QSpinBox *m_offset = nullptr;
    QDoubleSpinBox *m_speed = nullptr;
    QPushButton *m_searchButton = nullptr;
    QPushButton *m_compareButton = nullptr;
    QListWidget *m_resultsList = nullptr;
    QPlainTextEdit *m_editor = nullptr;
    QLabel *m_status = nullptr;
    QLabel *m_fileLabel = nullptr;
    QLabel *m_trackLabel = nullptr;
    QLabel *m_capabilityLabel = nullptr;
};
