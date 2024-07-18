%{!?directory:%define directory /usr}

%define buildroot %{_tmppath}/%{name}
%define packckname stbimage

Name:          tcl-stbimage
Summary:       Tcl extension for stb_image
Version:       1.1
Release:       0
License:       MIT
Group:         Development/Libraries/Tcl
Source:        %{name}-%{version}.tar.gz
URL:           https://github.com/ray2501/tcl-stbimage
BuildRequires: autoconf
BuildRequires: make
BuildRequires: tcl-devel >= 8.6
Requires:      tcl >= 8.6
BuildRoot:     %{buildroot}

%description
It is a Tcl extension for stb_image. This package is using stb_image Easy-to-use
API to load (ex. jpg/png/tga/bmp), resize and write (ex. jpg/png/tga/bmp) image.

%prep
%setup -q -n %{name}-%{version}

%build
CFLAGS="%optflags" ./configure \
	--prefix=%{directory} \
	--exec-prefix=%{directory} \
	--libdir=%{directory}/%{_lib}
make 

%install
make DESTDIR=%{buildroot} pkglibdir=%{tcl_archdir}/%{packckname}%{version} install

%clean
rm -rf %buildroot

%files
%defattr(-,root,root)
%doc README.md LICENSE
%{tcl_archdir}
