#ifndef SIGNINGCERTIFICATELISTMODEL_H
#define SIGNINGCERTIFICATELISTMODEL_H

#include "gui/signatureguiutils.h"
#include "signaturepartutilsmodel.h"
#include <QAbstractListModel>

class SigningCertificateListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit SigningCertificateListModel(const QList<Okular::CertificateInfo> &certs, QObject *parent = nullptr);
    bool hasIcons() const;
    QVariant data(const QModelIndex &index, int role) const override;
    /** @returns the modelIndex for a given nick or invalid if no found*/
    QModelIndex indexForNick(const QString &nick) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    /** @return minWidth in character count needed to calculate size */
    qsizetype minWidth() const;
    SignaturePartUtils::CertificateTypes types() const;

private:
    const QList<Okular::CertificateInfo> m_certs;
    bool m_hasIcons = false;
    qsizetype m_minWidth = -1;
    SignaturePartUtils::CertificateTypes m_types = SignaturePartUtils::CertificateType::None;
};

#endif // SIGNINGCERTIFICATELISTMODEL_H
