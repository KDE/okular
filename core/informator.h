
/**
 * @short The source of information about the Generator
 * 
 * The goal of this class is to provide per Generator settings,
 * about data and other stuff not related with generating okular 
 * content, but still needed for a full-featured plugin system.
 *
 */

class Informator
{
    virtual QString iconName() = 0;
    virtual void addConfigurationPages ( KConfigDialog* cfg ) = 0;
    virtual KAboutData about();
    virtual QStringList mimetypes() = 0;
}
