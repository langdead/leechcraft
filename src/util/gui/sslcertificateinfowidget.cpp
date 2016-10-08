/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Boost Software License - Version 1.0 - August 17th, 2003
 *
 * Permission is hereby granted, free of charge, to any person or organization
 * obtaining a copy of the software and accompanying documentation covered by
 * this license (the "Software") to use, reproduce, display, distribute,
 * execute, and transmit the Software, and to prepare derivative works of the
 * Software, and to permit third-parties to whom the Software is furnished to
 * do so, all subject to the following:
 *
 * The copyright notices in the Software and this entire statement, including
 * the above license grant, this restriction and the following disclaimer,
 * must be included in all copies of the Software, in whole or in part, and
 * all derivative works of the Software, unless such copies or derivative
 * works are solely in the form of machine-executable object code generated by
 * a source language processor.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 **********************************************************************/

#include "sslcertificateinfowidget.h"
#include <QSslCertificate>
#include <QDateTime>
#include "ui_sslcertificateinfowidget.h"

namespace LeechCraft
{
namespace Util
{
	SslCertificateInfoWidget::SslCertificateInfoWidget (QWidget *parent)
	: QWidget { parent }
	, Ui_ { std::make_shared<Ui::SslCertificateInfoWidget> () }
	{
		Ui_->setupUi (this);
	}

	void SslCertificateInfoWidget::SetCertificate (const QSslCertificate& cert)
	{
		auto setSubjectInfo = [&cert] (QLabel *label, QSslCertificate::SubjectInfo key)
		{
#if QT_VERSION < 0x050000
			label->setText (cert.subjectInfo (key));
#else
			label->setText (cert.subjectInfo (key).join ("; "));
#endif
		};
		auto setIssuerInfo = [&cert] (QLabel *label, QSslCertificate::SubjectInfo key)
		{
#if QT_VERSION < 0x050000
			label->setText (cert.issuerInfo (key));
#else
			label->setText (cert.issuerInfo (key).join ("; "));
#endif
		};

		setSubjectInfo (Ui_->SubjectCommonName_, QSslCertificate::CommonName);
		setSubjectInfo (Ui_->SubjectOrganization_, QSslCertificate::Organization);
		setSubjectInfo (Ui_->SubjectUnit_, QSslCertificate::OrganizationalUnitName);
		setSubjectInfo (Ui_->SubjectCountry_, QSslCertificate::CountryName);
		setSubjectInfo (Ui_->SubjectState_, QSslCertificate::StateOrProvinceName);
		setSubjectInfo (Ui_->SubjectCity_, QSslCertificate::LocalityName);
		setIssuerInfo (Ui_->IssuerCommonName_, QSslCertificate::CommonName);
		setIssuerInfo (Ui_->IssuerOrganization_, QSslCertificate::Organization);
		setIssuerInfo (Ui_->IssuerUnit_, QSslCertificate::OrganizationalUnitName);
		setIssuerInfo (Ui_->IssuerCountry_, QSslCertificate::CountryName);
		setIssuerInfo (Ui_->IssuerState_, QSslCertificate::StateOrProvinceName);
		setIssuerInfo (Ui_->IssuerCity_, QSslCertificate::LocalityName);

		Ui_->SerialNumber_->setText (cert.serialNumber ());
		Ui_->Md5_->setText (cert.digest (QCryptographicHash::Md5).toHex ());
		Ui_->Sha1_->setText (cert.digest (QCryptographicHash::Sha1).toHex ());

		Ui_->StartDate_->setText (QLocale {}.toString (cert.effectiveDate ()));
		Ui_->EndDate_->setText (QLocale {}.toString (cert.expiryDate ()));
	}
}
}
