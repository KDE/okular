########    #######    ########    #######    ########    ########
##     / / / /    License    \ \ \ \ 
##    Copyleft culture, Copyright (C) is prohibited here
##    This work is licensed under a CC BY-SA 4.0
##    Creative Commons Attribution-ShareAlike 4.0 License
##    Refer to the http://creativecommons.org/licenses/by-sa/4.0/
########    #######    ########    #######    ########    ########
##    / / / /    Code Climate    \ \ \ \ 
##    Language = rpm spec
##    Indent = space;    4 chars;
########    #######    ########    #######    ########    ########

## // In4 \\
%global _obs_path %(ls -d %{_sourcedir}/*.obs)
%global _obs_helper     %(echo %{_obs_path}|sed -e "s/.*\\///")

%global _my_release_name    %(echo %{_obs_helper}|sed -e "s/-__.*//")
%global _my_release_custname    %(echo %{_obs_helper}|sed -e "s/.*__CustName@//" -e "s/__.*//")
%global _my_release_version %(echo %{_obs_helper}|sed -e "s/.*__Ver@//" -e "s/__.*//")
%global _my_release_date    %(echo %{_obs_helper}|sed -e "s/.*__RelDate@//" -e "s/__.*//")
%global _my_release_tag     %(echo %{_obs_helper}|sed -e "s/.*__Hash@//" -e "s/__.*//")

#MANDATORY FIELDS
Version: %{_my_release_version}
Name:    %{_my_release_name}
Release: 1
## \\ In4 //

## SOURCES & PATCHES
#Source0: 
#Patch0:        
##

%bcond_without lang
%define kf5_version 5.26.0
# Latest stable Applications (e.g. 17.08 in KA, but 17.11.80 in KUA)
%{!?_kapp_version: %global _kapp_version %(echo %{version}| awk -F. '{print $1"."$2}')}

Summary:        Document Viewer
License:        GPL-2.0+
Group:          Productivity/Office/Other
Url:            http://www.kde.org
Source:         %{name}-%{version}.tar.xz
BuildRequires:  chmlib-devel
BuildRequires:  extra-cmake-modules
BuildRequires:  freetype2-devel
BuildRequires:  kactivities5-devel
BuildRequires:  karchive-devel
BuildRequires:  kbookmarks-devel
BuildRequires:  kcompletion-devel
BuildRequires:  kconfig-devel
BuildRequires:  kconfigwidgets-devel
BuildRequires:  kcoreaddons-devel
BuildRequires:  kdbusaddons-devel
BuildRequires:  kdoctools-devel
BuildRequires:  kf5-filesystem
BuildRequires:  khtml-devel
BuildRequires:  kiconthemes-devel
BuildRequires:  kio-devel
BuildRequires:  kjs-devel
BuildRequires:  kparts-devel
BuildRequires:  kpty-devel
BuildRequires:  kwallet-devel
BuildRequires:  kwindowsystem-devel
BuildRequires:  libdjvulibre-devel
BuildRequires:  libepub-devel
BuildRequires:  libjpeg-devel
BuildRequires:  libkexiv2-devel
BuildRequires:  libpoppler-qt5-devel
BuildRequires:  libqca-qt5-devel
BuildRequires:  libspectre-devel
BuildRequires:  libtiff-devel
BuildRequires:  mobipocket-devel
BuildRequires:  phonon4qt5-devel
BuildRequires:  pkgconfig
BuildRequires:  threadweaver-devel
BuildRequires:  zlib-devel
BuildRequires:  pkgconfig(Qt5Core) >= 5.6.0
BuildRequires:  pkgconfig(Qt5DBus) >= 5.6.0
BuildRequires:  pkgconfig(Qt5PrintSupport) >= 5.6.0
BuildRequires:  pkgconfig(Qt5Qml) >= 5.6.0
BuildRequires:  pkgconfig(Qt5Quick) >= 5.6.0
BuildRequires:  pkgconfig(Qt5Svg) >= 5.6.0
BuildRequires:  pkgconfig(Qt5Test) >= 5.6.0
BuildRequires:  pkgconfig(Qt5TextToSpeech) >= 5.6.0
BuildRequires:  pkgconfig(Qt5Widgets) >= 5.6.0
Obsoletes:      okular5 < %{version}
Provides:       okular5 = %{version}
%if %{with lang}
Recommends:     %{name}-lang
%endif
BuildRoot:      %{_tmppath}/%{name}-%{version}-build

%description
Document viewing program; supports document in PDF, PS and
many other formats.

%package devel
Summary:        Document Viewer - Development Files
Group:          Development/Libraries/KDE
Requires:       %{name} = %{version}
Requires:       cmake(KF5Config)
Requires:       cmake(KF5CoreAddons)
Requires:       cmake(KF5XmlGui)
Requires:       cmake(Qt5Core) >= 5.6.0
Requires:       cmake(Qt5PrintSupport) >= 5.6.0
Requires:       cmake(Qt5Widgets) >= 5.6.0
Obsoletes:      okular5-devel < %{version}
Provides:       okular5-devel = %{version}

%description devel
Document viewing program; supports document in various formats

%if %{with lang}
%lang_package
%endif


%prep
## // In4 \\
cd %{_obs_path}
## \\ In4 //

#%patch0 -p1

%build
## // In4 \\
mv %{_obs_path} %{_sourcedir}/../BUILD && cd %{_sourcedir}/../BUILD/*.obs
## \\ In4 //

%cmake_kf5 -d build -- -DBUILD_TESTING=ON
%make_jobs


%install
## // In4 \\
cd %{buildroot}/../../BUILD/*.obs
## \\ In4 //

%make_install -C build
%if %{with lang}
  %find_lang %{name} --with-man --all-name
  %kf5_find_htmldocs
%endif

rm -rfv %{buildroot}/%{_kf5_applicationsdir}/org.kde.mobile*

%post   -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%files
%defattr(-,root,root)
%doc COPYING*
%config %{_kf5_configdir}/okular.categories
%dir %{_kf5_appstreamdir}
%dir %{_kf5_htmldir}
%dir %{_kf5_htmldir}/en
%doc %lang(en) %{_kf5_htmldir}/en/*/
%doc %{_kf5_mandir}/man1/okular.*
%{_kf5_sharedir}/kpackage/
%{_kf5_applicationsdir}/*.desktop
%{_kf5_appstreamdir}/org.kde.okular*.xml
%{_kf5_bindir}/okular
%{_kf5_configkcfgdir}/
%{_kf5_iconsdir}/hicolor/*/*/okular.*
%{_kf5_kxmlguidir}/
%{_kf5_libdir}/libOkular5Core.so.*
%{_kf5_plugindir}/
%{_kf5_qmldir}/
%{_kf5_servicesdir}/
%{_kf5_servicetypesdir}/
%{_kf5_sharedir}/kconf_update
%{_kf5_sharedir}/okular

%files devel
%defattr(-,root,root)
%doc COPYING*
%{_kf5_cmakedir}/Okular5/
%{_kf5_libdir}/libOkular5Core.so
%{_kf5_prefix}/include/okular/

%if %{with lang}
%files lang -f %{name}.lang
%doc COPYING*
%endif

%changelog
