#include "signingcertificatelistmodel.h"
#include <KLocalizedString>
#include <QRegularExpression>

SigningCertificateListModel::SigningCertificateListModel(const QList<Okular::CertificateInfo> &certs, QObject *parent)
    : QAbstractListModel {parent}
    , m_certs {certs}
{
    for (const auto &cert : m_certs) {
        switch (cert.keyLocation()) {
        case Okular::CertificateInfo::KeyLocation::Computer:
        case Okular::CertificateInfo::KeyLocation::HardwareToken:
            m_hasIcons = true;
            break;
        case Okular::CertificateInfo::KeyLocation::Unknown:
        case Okular::CertificateInfo::KeyLocation::Other:
            break;
        }
        if (cert.isQualified()) {
            m_types |= SignaturePartUtils::CertificateType::QES;
        } else if (cert.certificateType() == Okular::CertificateInfo::CertificateType::PGP) {
            m_types |= SignaturePartUtils::CertificateType::PGP;
        } else {
            m_types |= SignaturePartUtils::CertificateType::SMime;
        }
        m_minWidth = std::max(
            m_minWidth,
            std::max(cert.nickName().size(),
                     cert.subjectInfo(Okular::CertificateInfo::EmailAddress, Okular::CertificateInfo::EmptyString::Empty).size() + cert.subjectInfo(Okular::CertificateInfo::CommonName, Okular::CertificateInfo::EmptyString::Empty).size()));
    }
    // this would mean an empty list and that code should not have been called
    Q_ASSERT(m_types != SignaturePartUtils::CertificateType::None);
}

bool SigningCertificateListModel::hasIcons() const
{
    return m_hasIcons;
}

SignaturePartUtils::CertificateTypes SigningCertificateListModel::types() const
{
    return m_types;
}

qsizetype SigningCertificateListModel::minWidth() const
{
    return m_minWidth;
}

QModelIndex SigningCertificateListModel::indexForNick(const QString &nick) const
{
    for (qsizetype i = 0; i < m_certs.size(); i++) {
        if (m_certs.at(i).nickName() == nick) {
            return index(i);
        }
    }
    return {};
}

int SigningCertificateListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_certs.size();
}

QVariant SigningCertificateListModel::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index, QAbstractItemModel::CheckIndexOption::IndexIsValid | QAbstractItemModel::CheckIndexOption::ParentIsInvalid)) {
        return {};
    }
    const Okular::CertificateInfo &cert = m_certs.at(index.row());
    switch (role) {
    case SignaturePartUtils::CommonNameRole:
        return cert.subjectInfo(Okular::CertificateInfo::CommonName, Okular::CertificateInfo::EmptyString::Empty);
    case SignaturePartUtils::EmailRole:
        return cert.subjectInfo(Okular::CertificateInfo::EmailAddress, Okular::CertificateInfo::EmptyString::Empty);
    case SignaturePartUtils::NickRole:
        return cert.nickName();
    case SignaturePartUtils::NickDisplayRole: {
        QString nick = cert.nickName();

        if (cert.backend() == Okular::CertificateInfo::Backend::Gpg) {
            static const auto group4 = QRegularExpression(QStringLiteral("(....)"));
            nick = nick.replace(group4, QStringLiteral("\\1 ")).trimmed();
        }
        return nick;
    }
    case SignaturePartUtils::NameEmailDisplayRole: {
        auto email = cert.subjectInfo(Okular::CertificateInfo::EmailAddress, Okular::CertificateInfo::EmptyString::Empty);
        auto name = cert.subjectInfo(Okular::CertificateInfo::CommonName, Okular::CertificateInfo::EmptyString::Empty);
        if (!email.isEmpty()) {
            return QStringLiteral("%1 <%2>").arg(name, email);
        }
        return name;
    }
    case SignaturePartUtils::CertRole:
        return QVariant::fromValue(cert);
    case SignaturePartUtils::TypeRole: {
        if (cert.isQualified()) {
            return i18nc("Qualified electronic signature, see wikipedia", "Qualified");
        }
        if (cert.certificateType() == Okular::CertificateInfo::CertificateType::PGP) {
            return i18n("PGP");
        }
        return {};
    }
    case Qt::ToolTipRole:
        return cert.subjectInfo(Okular::CertificateInfo::DistinguishedName, Okular::CertificateInfo::EmptyString::Empty);
    case Qt::DecorationRole:
        switch (cert.keyLocation()) {
        case Okular::CertificateInfo::KeyLocation::Computer:
            return QIcon::fromTheme(QStringLiteral("view-certificate"));
        case Okular::CertificateInfo::KeyLocation::HardwareToken:
            /* Better icon requested in https://bugs.kde.org/show_bug.cgi?id=428278*/
            return QIcon::fromTheme(QStringLiteral("auth-sim"));
        case Okular::CertificateInfo::KeyLocation::Unknown:
        case Okular::CertificateInfo::KeyLocation::Other:
            return {};
        }
    }
    return {};
}

bool FilterSigningCertificateTypeListModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    if (m_types == SignaturePartUtils::CertificateType::None) {
        return true;
    }
    if (!sourceModel()) {
        return false;
    }
    auto data = sourceModel()->index(sourceRow, 0, sourceParent);
    if (!data.isValid()) {
        return false;
    }
    auto cert = data.data(SignaturePartUtils::CertRole).value<Okular::CertificateInfo>();
    if (m_types.testFlag(SignaturePartUtils::CertificateType::QES) && cert.isQualified()) {
        return true;
    }
    if (m_types.testFlag(SignaturePartUtils::CertificateType::PGP) && cert.certificateType() == Okular::CertificateInfo::CertificateType::PGP) {
        return true;
    }
    return false;
}

void FilterSigningCertificateTypeListModel::setAllowedTypes(SignaturePartUtils::CertificateTypes types)
{
    if (types != m_types) {
        m_types = types;
        invalidateRowsFilter();
    }
}
