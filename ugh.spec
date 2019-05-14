Summary:	UGH http server
Name:		ugh	
Version:	0.1.15
Release:	1%{?dist}
Group:		Networking/Daemons
License:	GPL
Source:		ugh-%{version}.tar.gz
BuildRoot:	%{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
BuildRequires:  Judy-devel libev-devel cmake gcc-c++

%package devel
Summary:        Header files and development documentation for %{name}
Group:          Development/Libraries
Requires:       %{name} = %{version}-%{release}

%description
UGH http server

%description devel
UGH header files.

This package contains the header files, static libraries and development
documentation for %{name}. If you like to develop modules for %{name},
you will need to install %{name}-devel.

%prep
%setup -q -n %{name}-%{version}

%build
%cmake .
make %{?_smp_mflags}

%install
rm -rf %{buildroot}
mkdir %{buildroot}
make install DESTDIR=%{buildroot}

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root,-)
%{_bindir}/ugh
%{_bindir}/ugh_test_resolver
%{_prefix}/etc/ugh.cfg
%{_prefix}/etc/ugh_example/config.cfg
%{_libdir}/ugh/libugh_example.so

%files devel
%defattr(-,root,root,-)
%{_includedir}/ugh/autoconf.h
%{_includedir}/ugh/aux/buffer.h
%{_includedir}/ugh/aux/config.h
%{_includedir}/ugh/aux/daemon.h
%{_includedir}/ugh/aux/gmtime.h
%{_includedir}/ugh/aux/hashes.h
%{_includedir}/ugh/aux/logger.h
%{_includedir}/ugh/aux/memory.h
%{_includedir}/ugh/aux/random.h
%{_includedir}/ugh/aux/resolver.h
%{_includedir}/ugh/aux/socket.h
%{_includedir}/ugh/aux/string.h
%{_includedir}/ugh/aux/system.h
%{_includedir}/ugh/config.h
%{_includedir}/ugh/coro/coro.h
%{_includedir}/ugh/coro_ucontext/coro_ucontext.h
%{_includedir}/ugh/module.h
%{_includedir}/ugh/resolver.h
%{_includedir}/ugh/ugh.h

%changelog

* Mon Sep 13 2019 Ruslan Osmanov <rrosmanov@gmail.com> - 0.1.15-1
+ Added support for HTTP statuses 417 and 451

* Mon Jan 05 2015 Alexander Pankov <pianist@usrsrc.ru> - 0.1.8-1
+ First RPM build

