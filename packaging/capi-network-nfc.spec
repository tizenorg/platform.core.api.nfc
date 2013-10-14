Name:       capi-network-nfc
Summary:    NFC Core API
Version:    0.1.4
Release:    0
Group:      Network & Connectivity/NFC
License:    Apache-2.0
Source0:    %{name}-%{version}.tar.gz
Source1001: 	%{name}.manifest
BuildRequires:  cmake
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(gobject-2.0)
BuildRequires:  nfc-client-lib-devel
BuildRequires:  nfc-common-devel
BuildRequires:  pkgconfig(capi-base-common)
BuildRequires:  pkgconfig(vconf)
BuildRequires:  pkgconfig(ecore-x)
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig

%description
A library for Tizen NFC Core API.

%package devel
Summary:  NFC Core API (devel)
Group:    Network & Connectivity/Development
Requires: %{name} = %{version}-%{release}

%description devel
This package contains the development files for %{name}.


%prep
%setup -q
cp %{SOURCE1001} .


%build
MAJORVER=`echo %{version} | awk 'BEGIN {FS="."}{print $1}'`
%cmake . -DFULLVER=%{version} -DMAJORVER=${MAJORVER}

make %{?jobs:-j%jobs}

%install
%make_install

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig


%files
%manifest %{name}.manifest
%{_libdir}/libcapi-network-nfc.so.*
%license LICENSE.APLv2

%files devel
%manifest %{name}.manifest
%{_includedir}/network/*.h
%{_libdir}/pkgconfig/*.pc
%{_libdir}/libcapi-network-nfc.so
