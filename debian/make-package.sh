#!/bin/bash

function get_package_version {
    # compute metaserver dependency
    PACKAGE_VERSION=$(dpkg -l $1 | grep ^ii | tr -s ' ' ' ' | cut -d' ' -f3)

    if test -z "${PACKAGE_VERSION}"; then
        # Error
        echo "Package $1 is not installed. Bailing out" >> /dev/stderr
        exit -1
    fi

    echo ${PACKAGE_VERSION}
}

function buildDepends() {

    function listPackages() {
        (
            for x in $*; do
                for a in `ldd "$x" | cut -f 2- -d"/" | cut -f 1 -d"("`; do
                    echo "$(dpkg -S "`readlink -f "/$a"`" | cut -f 1 -d:)"
                done
            done
        ) | sort -u
    }

    function depends() {
        (
            for a in `listPackages $*`; do
                if [ -f "/var/lib/dpkg/info/$a.shlibs" ]; then
                    cat "/var/lib/dpkg/info/$a.shlibs" | grep -v "^[^:]*: " | grep " $a " \
                        | cut -f 3- -d" " | sed "s/\(.*\)/\1, /g"
                fi
            done
        ) | sort -u
    }

    depends $* | tr -d "\n" | sed "s/,\s*$//g"
    echo
}

if test -z "${DEB_PCK_NAME}"; then
    echo "Do not run me alone! I'm happy to be included in your packager."
    exit 1
fi

# Make extra depend
EXTRA_DEPEND=""
for pkg in ${EXTRA_DEPEND_PACKAGES}; do
    VERSION=$(get_package_version ${pkg}) || exit 1
    EXTRA_DEPEND="${EXTRA_DEPEND} ${pkg} (>= ${VERSION}),"
done

if [ -d ${WORK_DIR}/bin ]; then
    # build extra depend
    if grep -q "use Dpkg::Control" "`which dpkg-shlibdeps`" ; then
        SH_DEPEND=$(buildDepends ${WORK_DIR}/bin/*)
    else
        SH_DEPEND=$(dpkg-shlibdeps -O ${WORK_DIR}/bin/* | \
            gawk '{match($0, /^.*Depends=(.*)$/, a); print a[1]}')
    fi
    EXTRA_DEPEND="${EXTRA_DEPEND} ${SH_DEPEND}"
fi
if [ -d ${WORK_DIR}/libexec ]; then
    # build extra depend
    if grep -q "use Dpkg::Control" "`which dpkg-shlibdeps`" ; then
        SH_DEPEND=$(buildDepends ${WORK_DIR}/libexec/*.so)
    else
        SH_DEPEND=$(dpkg-shlibdeps -O ${WORK_DIR}/libexec/*.so | \
            gawk '{match($0, /^.*Depends=(.*)$/, a); print a[1]}')
    fi
    EXTRA_DEPEND="${EXTRA_DEPEND} ${SH_DEPEND}"
fi

# Make symlink to logy dir
if [ -d ${WORK_DIR}/log ]; then
    mkdir -p ${DEBIAN_BASE}/www/logy/www/
    ln -s ${PROJECT_DIR}/log ${DEBIAN_BASE}/www/logy/www/${DEB_PCK_NAME}
fi

# Make supervisor autodescriptor if group defined
if test -n "${SUPERVISOR_GROUP}"; then
    AUTODESCRIPTOR="${DEBIAN_BASE}/www/supervisor/server/conf/autodescriptor"
    mkdir -p ${AUTODESCRIPTOR}
    echo ${SUPERVISOR_GROUP} > ${AUTODESCRIPTOR}/${DEB_PCK_NAME}
    chmod 644 ${AUTODESCRIPTOR}/${DEB_PCK_NAME}
fi

# Make PID directory if its owner defined
if test -n "${PIDDIR_OWNER}"; then
    PIDDIR="${DEBIAN_BASE}/var/run/${DEB_PCK_NAME}"
    mkdir -p ${PIDDIR}
    chown "${PIDDIR_OWNER}" ${PIDDIR}
    chmod 755 ${PIDDIR}
fi

# Compile python files
for DIR in ${PYTHON_FILES_ROOT}; do
    (cd "${DIR}"
    for file in `find . -name "*.py" -print`;
    do
        echo "byte-compiling '"${file}"'"
        # vytvorit .pyo (je to nutne)
        echo "import py_compile; py_compile.compile('"${file}"')" | ${PYTHON} -O
        # vytvorit .pyc
        echo "import py_compile; py_compile.compile('"${file}"')" | ${PYTHON}
        # nastavit prava pro .py?
        chmod 644 ${file}?
    done)
done


### packaging

cp $DEB_PCK_NAME.control $DEBIAN_BASE/DEBIAN/control

# konfiguraky, ktere jsou ve dvojicich .go a .ng (pro replikaci) a je potreba na
# ne pri postinstu udelat symlink
CONFPREFIXES=(`sed -n 's/\.ng\.conf$//gp' $DEB_PCK_NAME.conffiles`)

for file in postinst preinst prerm postrm; do
    if test -f ${DEB_PCK_NAME}.${file}; then
        cp -f ${DEB_PCK_NAME}.$file ${DEBIAN_BASE}/DEBIAN/${file}
        chmod 0755 ${DEBIAN_BASE}/DEBIAN/${file}

        if test -n "${CONFPREFIXES}"; then
            case "$file" in
                postinst | prerm)
                    cat replication.$file >> ${DEBIAN_BASE}/DEBIAN/${file}
                    echo "${file}Links ${CONFPREFIXES[@]}" >> ${DEBIAN_BASE}/DEBIAN/${file}
                    ;;
            esac
        fi
    else
        if test -n "${CONFPREFIXES}"; then
            case "$file" in
                postinst | prerm)
                    echo "Missing $file file." >&2
                    exit 1
                    ;;
            esac
        fi
    fi
done

test -f ${DEB_PCK_NAME}.conffiles && \
    cp -f ${DEB_PCK_NAME}.conffiles ${DEBIAN_BASE}/DEBIAN/conffiles

for f in `find $DEBIAN_BASE -path "*CVS*"`; do
    rm -R $f 2>/dev/null
done;

SIZEDU=`du -sk "$DEBIAN_BASE" | awk '{ print $1}'`
SIZEDIR=`find "$DEBIAN_BASE" -type d | wc | awk '{print $1}'`
SIZE=$[ $SIZEDU - $SIZEDIR ]

VERSION=$(<build/version)

sed     -e "s/__VERSION__/$VERSION/" \
    -e "s/__PACKAGE__/$DEB_PCK_NAME/" \
    -e "s/__MAINTAINER__/$MAINTAINER/" \
    -e "s/__SIZE__/$SIZE/" \
    -e "s/__ARCHITECTURE__/$(dpkg --print-architecture)/" \
    -e "s/__EXTRA_DEPEND__/$EXTRA_DEPEND/" \
    $DEB_PCK_NAME.control > tmp/$DEB_PCK_NAME/DEBIAN/control

# Vytvori a prejmenuje balicek
dpkg --build $DEBIAN_BASE
dpkg-name -o $DEBIAN_BASE.deb
rm -r $DEBIAN_BASE
