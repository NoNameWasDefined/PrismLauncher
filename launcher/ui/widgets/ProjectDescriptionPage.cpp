// File overview: Implementation of ProjectDescriptionPage.h. Text browser for project descriptions (HTML/Markdown) with
// image loading.

#include "ProjectDescriptionPage.h"

#include "VariableSizedImageObject.h"

#include <QDebug>

ProjectDescriptionPage::ProjectDescriptionPage(QWidget* parent) : QTextBrowser(parent), m_image_text_object(new VariableSizedImageObject)
{
    m_image_text_object->setParent(this);
    document()->documentLayout()->registerHandler(QTextFormat::ImageObject, m_image_text_object.get());
}

void ProjectDescriptionPage::setMetaEntry(QString entry)
{
    if (m_image_text_object)
        m_image_text_object->setMetaEntry(entry);
}

void ProjectDescriptionPage::flush()
{
    if (m_image_text_object)
        m_image_text_object->flush();
}
