Name:       capi-network-nfc
Summary:    A NFC library in Native API
Version:    0.2.4
Release:    0
Group:      Network & Connectivity/NFC
License:    Apache-2.0
Source0:    %{name}-%{version}.tar.gz
BuildRequires:  cmake
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(gobject-2.0)
BuildRequires:  pkgconfig(capi-base-common)
BuildRequires:  pkgconfig(vconf)
BuildRequires:  pkgconfig(capi-system-info)
BuildRequires:  pkgconfig(nfc-common-lib)

Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig

%description
A library for Tizen NFC Native API.

%package devel
Summary:  A NFC library in Native API (Development)
Group:    Network & Connectivity/Development
Requires: %{name} = %{version}-%{release}

%description devel
This package contains the development files for %{name}.


%prep
%setup -q


%build
export CFLAGS="$CFLAGS -DTIZEN_ENGINEER_MODE"
export CXXFLAGS="$CXXFLAGS -DTIZEN_ENGINEER_MODE"
export FFLAGS="$FFLAGS -DTIZEN_ENGINEER_MODE"

MAJORVER=`echo %{version} | awk 'BEGIN {FS="."}{print $1}'`
cmake . -DCMAKE_INSTALL_PREFIX=/usr -DFULLVER=%{version} -DMAJORVER=${MAJORVER} -DCMAKE_LIB_DIR=%{_libdir}

make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/usr/share/license
cp -af %{_builddir}/%{name}-%{version}/packaging/capi-network-nfc %{buildroot}/usr/share/license/

%make_install

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig


%files
%manifest capi-network-nfc.manifest
%{_libdir}/libcapi-network-nfc.so*
/usr/share/license/capi-network-nfc

%files devel
%{_includedir}/network/*.h
%{_libdir}/pkgconfig/*.pc
%{_libdir}/libcapi-network-nfc.so
