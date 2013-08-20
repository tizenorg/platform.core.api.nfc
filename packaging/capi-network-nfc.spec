#sbs-git:slp/api/nfc capi-network-nfc 0.0.1 14f15050f8f6ff8217421da04fa5d66e834e2016
Name:       capi-network-nfc
Summary:    A NFC library in SLP C API
Version:    0.1.0
Release:    0
Group:      API/C API
License:    Apache-2.0
Source0:    %{name}-%{version}.tar.gz
Source1001: 	%{name}.manifest
BuildRequires:  cmake
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(gobject-2.0)
BuildRequires:  pkgconfig(nfc)
BuildRequires:  pkgconfig(nfc-common-lib)
BuildRequires:  pkgconfig(capi-base-common)
BuildRequires:  pkgconfig(vconf)
BuildRequires:  pkgconfig(ecore-x)
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig

%description
A NFC library in SLP C API.


%package devel
Summary:  A NFC library in SLP C API -- Development Files
Group:    API/C API
Requires: %{name} = %{version}-%{release}

%description devel
A NFC library in SLP C API.

This package contains the development files for %{name}.


%prep
%setup -q
cp %{SOURCE1001} .


%build
MAJORVER=`echo %{version} | awk 'BEGIN {FS="."}{print $1}'`
%cmake . -DFULLVER=%{version} -DMAJORVER=${MAJORVER}

make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/usr/share/license
cp -af %{_builddir}/%{name}-%{version}/packaging/capi-network-nfc %{buildroot}/usr/share/license/

%make_install

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig


%files
%manifest %{name}.manifest
%{_libdir}/libcapi-network-nfc.so.*
/usr/share/license/capi-network-nfc

%files devel
%manifest %{name}.manifest
%{_includedir}/network/*.h
%{_libdir}/pkgconfig/*.pc
%{_libdir}/libcapi-network-nfc.so
