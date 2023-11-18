#!/usr/bin/env python3.8
# -*- coding: utf-8 -*-

"""
DISCLAIMER:
This script, which is a modified version of handin.py (https://handin.cs.clemson.edu/static/handin.py/), is for internal use only
and is not intended for external distribution. It has been upgraded to Python 3.8, includes
disabled SSL verification, and contains fixes for specific bugs encountered in the original
implementation.

The modifications are made for dylan mumm's personal use in the clemson school of computing and should not
be distributed or used outside of the intended context. The original utility is the property
of its respective owner(s), and this modified version adheres to the original licensing terms.

NO WARRANTY:
This software is provided 'as-is', without any express or implied warranty. In no event will
the authors be held liable for any damages arising from the use of this software. Use at your
own risk.

NOTICE:
Disabling SSL verification can lead to security risks. This modification was made to address
specific issues and is not recommended for general use. Ensure you understand the implications
of this change for your security and privacy.
"""


"""
Hand in homework and lab assignments via the command line.

The first time you run `handin` you may get a prompt to help generate a user
configuration file. If you need to run this again, use `handin --configure`.


See `handin --help` to get the full usage.

Examples
--------

Run an interactive session:

  $> handin main.c vec.c vec.h Makefile
  Generating first-time configuration.
  Default remote name (remote): example
  Enter submit URL: http://example.com/client/submit
  Default username (leave blank for none): myusername

  Submitting as 'myusername'
  Password:
  Bucket name: cpsc1010/asg01

Specify the bucket name over the command line:

  $> handin -b cpsc1010/asg01 main.c vec.c vec.h Makefile

Regenerate user configuration:

  $> handin --configure

Remotes
-------

Remotes are just easy-to-type names of URL's to upload files to. Normally you
set up a default remote (usually just named "remote") and never think about it
again. It is possible to use multiple remotes.

Configuration
-------------

This script searches `/etc/handin` and `~/.handin` for a file called `config`
which acts as the configuration file for the client. This file is generated
by `handin --configure` to given you an idea of what the syntax looks like. It
is similar to an INI file.

The user configuration file overrides the system one, and flags override
everything else. Also, `handin --configure` ignores generating preferences if
they already exist in the system configuration. This is to prevent users from
accidentally changing a setting that a sysadmin has previously configured.

The top section is the [handin] section which has some of the global settings.
In this section normally is the name of the default remote. Here is a basic
example:

  [handin]
  remote = remote

The valid attributes of this section are:

- remote: the name of the default remote to use
- username: the default username to try on all remotes
- use_system_username: (yes or no) whether or not to assume the system username
  across all remotes

The remainder of the sections are the names of remotes. Remotes only require
a URL to work, though there are several possible settings:

- url: the remote's submission URL
- name: a user-friendly name to output on the console
- username: a username override for this remote only
- use_system_username: (yes or no) use the system username for this remote
- cabundle: a path to a certificate authority bundle to use for SSL

Here is a simple example:

  [remote]
  url = http://example.com/client/submit
"""
import errno
import getpass
import json
import logging
import os
import pycurl
import sys
import tarfile
import tempfile
from configparser import ConfigParser
from io import StringIO

from functools import reduce
import operator

from optparse import OptionParser

from io import BytesIO


try:
    import curses
    import curses.panel
except ImportError:
    curses = None

PROG = os.path.basename(sys.argv[0])
USAGE = "%prog [-u user] [-b bucket] [options] file1 [file2...]"
VERSION = "0.1"
RELEASE = VERSION + ".3"
FORK = RELEASE + " (dyl fork)"
DESCRIPTION = "Hand in homework and lab assignments."
EPILOG = """\
The first time you run {prog} you may be prompted for a username and
information about where to send your work. If you need to change this, just run
`{prog} --configure` or edit the configuration file it generates at
`~/.handin/config`.
""".format(
    prog=PROG
)

LOG_LEVEL = logging.INFO
LOG_FORMAT = "%(tag)s: %(message)s"

DEFAULT_USER = getpass.getuser()

# name of default remote when not specified by user
DEFAULT_REMOTE = "remote"

# section name for configurable program options
CONFIG_SECTION = "handin"

SYSTEM_CONFIG_ROOT = "/etc/handin"
USER_CONFIG_ROOT = os.path.expanduser("~/.handin")
CONFIG_ROOTS = [SYSTEM_CONFIG_ROOT, USER_CONFIG_ROOT]


class Headers(object):
    """Storage object for header data returned by pycurl."""

    def __init__(self):
        self._headers = {}
        self._lastheader = None

    def write(self, buf):
        buf = buf.decode("iso-8859-1")
        if buf.endswith("\r\n"):
            buf = buf[:-2]
        else:  # this is not standard, but for the sake of robustness
            buf = buf.rstrip("\r\n")
        if buf.startswith("HTTP/"):
            pass
        elif ":" in buf:
            lhs, rhs = buf.split(":", 1)
            self._headers[lhs] = rhs.strip()
            self._lastheader = lhs
        elif self._lastheader and (buf.startswith(" ") or buf.startswith("\t")):
            self._headers[self._lastheader] += " " + buf

    def __getitem__(self, key):
        return self._headers[key]

    def __contains__(self, key):
        return key in self._headers

    def get(self, key, default=None):
        return self._headers.get(key, default)


class HandinError(Exception):
    pass


def main():
    parser = _get_parser()
    opt, args = parser.parse_args()
    logging.basicConfig(
        format=LOG_FORMAT,
        level=LOG_LEVEL - 10 * (opt.verbose - opt.quiet),
    )

    # run interactive configure if requested
    if opt.configure:
        config = _read_config(SYSTEM_CONFIG_ROOT)
        _interactive_configure(config)
        sys.exit(0)

    # list remote names if requested
    if opt.remotes:
        _list_remotes()
        sys.exit(0)

    if not args:
        sys.stderr.write("%s: no files specified\n\n" % parser.prog)
        parser.print_help(sys.stderr)
        sys.exit(1)

    # get configuration
    config = _get_config()

    # generate user configuration if site or user config does not exist
    if not any(os.path.exists(os.path.join(root, "config")) for root in CONFIG_ROOTS):
        _info("Generating first-time configuration.")
        _interactive_configure(config)
        config = _get_config()

    # set default remote if not given
    if not opt.remote:
        if config.has_option(CONFIG_SECTION, "remote"):
            opt.remote = config.get(CONFIG_SECTION, "remote")
        else:
            _fail("No configured default remote.")

    # check for valid remote names
    remotes = [s for s in config.sections() if s != CONFIG_SECTION]
    if opt.remote not in remotes:
        _fail("No remote named '%s'" % opt.remote)

    # check that remote has a url
    if not config.has_option(opt.remote, "url"):
        _fail("Remote '%s' has no URL configured." % opt.remote)

    # figure out what cabundle to use
    if opt.cabundle is None and config.has_option(opt.remote, "cabundle"):
        opt.cabundle = config.get(opt.remote, "cabundle")

    # Determine username if not given. First check for an entry in the chosen
    # remote's configuration, or check if we are configured to use the system
    # username. Do the same for global options. Finally, prompt if we could not
    # discover anything.
    if opt.user is None:
        if config.has_option(opt.remote, "username"):
            opt.user = config.get(opt.remote, "username")
        elif config.has_option(opt.remote, "use_system_username") and config.getboolean(
            opt.remote, "use_system_username"
        ):
            opt.user = DEFAULT_USER
        elif config.has_option(CONFIG_SECTION, "username"):
            opt.user = config.get(CONFIG_SECTION, "username")
        elif config.has_option(
            CONFIG_SECTION, "use_system_username"
        ) and config.getboolean(CONFIG_SECTION, "use_system_username"):
            opt.user = DEFAULT_USER
        else:
            opt.user = eval(input(f"Username ({DEFAULT_USER}): "))
            opt.user = opt.user or DEFAULT_USER
        if config.has_option(opt.remote, "name"):
            remote_name = config.get(opt.remote, "name")
        else:
            remote_name = opt.remote
        _info("Connecting to %s as '%s'" % (remote_name, opt.user))

    # always prompt for password
    opt.password = None
    while not opt.password:
        opt.password = getpass.getpass("Password: ")

    # prompt for bucket if needed
    if not opt.bucket:
        opt.bucket = _interactive_select_bucket(
            config, opt.remote, opt.user, opt.password, opt.cabundle
        )
        if opt.bucket is None:
            _warn("There are no buckets you can submit to.")
            _fail("Submission failed.")

    # let the user know what bucket they are submitting to
    _info("Bucket: %s" % opt.bucket)

    # build a tar.gz to send
    filename = _build_archive(args)

    # must remove 'filename' if we raise
    try:
        _info("Uploading...")

        d = [
            ("username", opt.user),
            ("password", opt.password),
            ("bucket", opt.bucket),
            ("file", (pycurl.FORM_FILE, filename)),
        ]
        try:
            status, data = _post(config, opt.remote, "submit", d, opt.cabundle)
        except HandinError as e:
            _warn(e.message)
            _fail("Submission failed.")

        # log server message to console
        log_level = logging.INFO
        if status >= 400:
            log_level = logging.WARN
        message = data.get("message", "(no message received)").splitlines()
        _log(log_level, "Status code: %d" % status)
        for line in message:
            _log(log_level, line, extra={"tag": opt.remote})

        # server returns 200 only if the submission was completed
        if status == 200:
            _info("Submission completed.")
            sys.exit(0)
        else:
            _fail("Submission failed.")
    finally:
        try:
            os.remove(filename)
        except OSError as e:
            if e.errno != errno.ENOENT:
                raise


def _read_config(root):
    """Read the configuration at the given configuration root."""
    config = ConfigParser()
    config.read(os.path.join(root, "config"))
    return config


def _get_curl(config, remote, command):
    """Return a boilerplate pycurl.Curl object with SSL verification disabled."""
    headers = [
        "User-Agent: %s/%s (%s)" % (PROG, RELEASE, sys.platform),
        "X-Handin-Version: %s" % VERSION,
    ]
    # Create a Headers object for storing response headers
    headers_obj = Headers()

    # Create a BytesIO buffer for storing response body
    response_buffer = BytesIO()

    curl = pycurl.Curl()
    curl.setopt(pycurl.URL, config.get(remote, "url") + "?command=" + command)
    curl.setopt(pycurl.HTTPHEADER, headers)

    # Set the callback function to store headers and body
    curl.setopt(pycurl.HEADERFUNCTION, headers_obj.write)
    curl.setopt(pycurl.WRITEFUNCTION, response_buffer.write)

    # Disable SSL certificate verification
    # curl.setopt(pycurl.SSL_VERIFYPEER, 0)

    # Return the curl object, headers object, and response buffer
    return curl, headers_obj, response_buffer


def _post(config, remote, command, postargs, cabundle):
    """Send a POST request to the given remote.

    Raises HandinError if there was some communication error.

    Returns decoded JSON data.

    Parameters:
      config: a configuration object
      remote: a remote name
      command: the command to execute ('list' or 'submit')
      postargs: pycurl-style post arguments
                (see pycurl.Curl.setopt(pycurl.HTTPPOST, ...))
    """
    response_buffer = BytesIO()  # Use BytesIO instead of StringIO
    headers = Headers()
    curl, headers_obj, response_buffer = _get_curl(config, remote, command)
    curl.setopt(pycurl.HTTPPOST, postargs)
    curl.setopt(pycurl.WRITEFUNCTION, response_buffer.write)
    curl.setopt(pycurl.HEADERFUNCTION, headers.write)
    if cabundle is not None:
        curl.setopt(pycurl.CAINFO, cabundle)

    curl.perform()

    # Check the response code and Content-Type header
    status = curl.getinfo(pycurl.HTTP_CODE)
    content_type = headers.get("Content-Type")
    if content_type is None:
        raise HandinError("Content-Type not provided (status: %d)" % status)
    elif content_type != "application/json":
        raise HandinError(
            "Content-Type (%s) not understood (status: %d)"
            % (
                content_type,
                status,
            )
        )

    # Attempt to parse the JSON response
    try:
        # Convert the response buffer from bytes to a string before loading JSON
        data = json.loads(response_buffer.getvalue().decode("utf-8"))
        return status, data
    except json.JSONDecodeError:
        # In case of a JSON decode error, log the response for debugging
        lines = response_buffer.getvalue().decode("utf-8").split("\n")
        for line in lines:
            _debug(line, extra={"tag": remote})
        raise HandinError("Could not parse response.")


def _get_config():
    """Read concatenation of system and user configuration."""
    config = ConfigParser()
    paths = [os.path.join(p, "config") for p in CONFIG_ROOTS]
    config.read(paths)
    return config


def _debug(msg, *args, **kw):
    """Shortcut for _log(logging.DEBUG, ...)"""
    _log(logging.DEBUG, msg, *args, **kw)


def _info(msg, *args, **kw):
    """Shortcut for _log(logging.INFO, ...)"""
    _log(logging.INFO, msg, *args, **kw)


def _warn(msg, *args, **kw):
    """Shortcut for _log(logging.WARN, ...)"""
    _log(logging.WARN, msg, *args, **kw)


def _log(lvl, msg, *args, **kw):
    """Log wrapper function."""
    extra = kw.setdefault("extra", {})
    extra.setdefault("tag", PROG)
    logging.log(lvl, msg, *args, **kw)


def _fail(msg, exitcode=1):
    """Shortcut for _warn() and sys.exit()."""
    _warn(msg)
    sys.exit(exitcode)


def _list_remotes():
    """Print remote names to standard out."""
    config = _get_config()
    for r in config.sections():
        if r == CONFIG_SECTION:
            continue
        print(r)


def _get_menu_choices(menu):
    """Get a tuple of menu choices.

    Returns a tuple of the menu selection ID's found in the given menu.

    Raises KeyError if a necessary menu value did not exist. This includes 'type'
    for all menus, 'choices' for type=menu, and 'id' for type=selection.

    Parameters:
      - menu (dict): the menu to search
    """
    import operator

    if menu["type"] == "selection":
        return (menu["id"],)
    elif menu["type"] == "menu":
        choices = [
            _get_menu_choices(submenu) for submenu in list(menu["choices"].values())
        ]
        return reduce(operator.add, choices, ())
    else:
        raise HandinError("Unknown item type: %s" % menu["type"])


def _print_menu_choices(menu, key, choices, level=0):
    """Print a menu recursively.

    Raises HandinError if a menu type is unhandled.

    Raises KeyError if a necessary menu value did not exist. This includes 'type'
    for all menus, 'choices' for type=menu, and 'id' for type=selection.

    Parameters:
      - menu (dict): a menu to print
      - key (str): a fallback value for display_name (generally it is the key
                   used to access this menu from the parent menu)
      - choices (dict): maps selection ID's to menu entry numbers. These will
                        be printed next to their corresponding selections.
    """
    indent = "    " * level

    if menu["type"] == "selection":
        print(f"{indent} [{choices[menu['id']]}] {menu.get('display_name', key)}")
    elif menu["type"] == "menu":
        print(f"{indent}{menu.get('display_name', key)}")
        for k in menu["choices"]:
            _print_menu_choices(menu["choices"][k], k, choices, level + 1)
    else:
        raise HandinError(f"Unknown item type: {menu['type']}")


def _interactive_select_bucket(config, remote, username, password, cabundle):
    """Interactively select a bucket.

    This queries the given remote for a list of buckets a user can submit to
    and presents the hierarchical menu that results. If for some reason the menu
    could not be understood, fall back on a simple prompt.

    Returns None if no buckets are available or a (possibly invalid) bucket ID
    the user has chosen to submit to.

    Parameters:
      - config: a configuration object
      - remote: the remote name
      - username: the username to log in as
      - password: password for the user
    """
    d = [
        ("username", username),
        ("password", password),
    ]
    try:
        status, data = _post(config, remote, "list", d, cabundle)
        if status >= 400:
            _warn(
                "%s [%d]" % (data.get("message", "(unknown error)"), status),
                extra={"tag": remote},
            )
            _fail("Submission failed.")
        elif status != 200:
            if "message" in data:
                _warn(data["message"], extra={"tag": remote})
                raise HandinError()
            else:
                _debug(str(data), extra={"tag": remote})
                raise HandinError("Bucket list request unsuccessful.")
        menu = data["menu"]
        if curses:
            result = _interactive_curses_menu(menu)
        else:
            result = _interactive_static_menu(menu)
        # pycurl cannot handle unicode
        if result is not None:
            result = str(result)
        return result
    except HandinError as e:
        if e.message:
            _warn(e.message)
    except KeyError:
        _warn("Could not parse bucket menu from server.")
        _debug("Exception info:", exc_info=True)

    # fall back on prompting user
    bucket = None
    while not bucket:
        bucket = input("Bucket name: ")
    return bucket


def _interactive_static_menu(menu):
    choices = dict(enumerate(_get_menu_choices(menu), 1))
    if not choices:
        return None
    choices = dict((str(k), v) for k, v in list(choices.items()))
    rchoices = dict((v, k) for k, v in list(choices.items()))
    _print_menu_choices(menu, "Menu", rchoices)
    choice = None
    while not choice:
        choice = input("Select bucket: ")
        if choice not in choices:
            print("Not a valid selection.")
            choice = None
    return choices[choice]


# See SO #14200721
class CursesMenu(object):
    def __init__(self, menu, stdscreen, pretitle="", auto_select_depth=2):
        self.screen = stdscreen
        self.window = stdscreen.subwin(0, 0)
        self.window.keypad(1)
        self.panel = curses.panel.new_panel(self.window)
        self.panel.hide()
        self.position = 0
        name = menu.get("display_name", "Menu")
        if pretitle:
            self.title = pretitle + " > " + name
        else:
            self.title = name
        while (
            auto_select_depth > 0
            and menu["type"] == "menu"
            and len(menu["choices"]) == 1
        ):
            auto_select_depth -= 1
            k, menu = list(menu["choices"].items())[0]
            name = menu.get("display_name", k)
            self.title += " > " + name
        self.prompt = menu.get("prompt", "Make a choice")
        curses.panel.update_panels()
        if menu["type"] == "selection":
            self.title = "Confirm submission"
            self.items = [(menu["id"], menu)]
        else:
            self.items = [
                (value.get("display_name", key), value)
                for key, value in sorted(menu["choices"].items())
            ]
        self.items.append(("Cancel", "CANCEL"))

    @property
    def choice(self):
        return self.items[self.position]

    def navigate(self, n):
        self.position = max(0, min(len(self.items) - 1, self.position + n))

    def display(self):
        self.panel.top()
        self.panel.show()
        self.window.clear()

        while True:
            self.window.refresh()
            curses.doupdate()
            for index, item in enumerate(self.items):
                if index == self.position:
                    mode = curses.A_REVERSE
                else:
                    mode = curses.A_NORMAL
                msg = "%d. %s" % (index + 1, item[0])
                self.window.addstr(0, 1, self.title, curses.A_NORMAL)
                self.window.addstr(2, 1, self.prompt + ":", curses.A_NORMAL)
                self.window.addstr(3 + index, 3, msg, mode)
            key = self.window.getch()
            if key in [curses.KEY_ENTER, ord("\n")]:
                item = self.choice[1]
                if item == "CANCEL":
                    break
                elif item["type"] == "menu":
                    submenu = CursesMenu(
                        item, self.screen, self.title, auto_select_depth=0
                    )
                    result = submenu.display()
                    if result != "CANCEL":
                        return result
                elif item["type"] == "selection":
                    return item["id"]
                else:
                    raise HandinError("Unknown item type: %s" % item["type"])
            elif key in (27, ord("q"), ord("Q")):
                break
            elif key == curses.KEY_UP:
                self.navigate(-1)
            elif key == curses.KEY_DOWN:
                self.navigate(1)
        self.window.clear()
        self.panel.hide()
        curses.panel.update_panels()
        curses.doupdate()
        return "CANCEL"


def _curses_menu_entry_point(menu, stdscreen):
    # Hide the terminal cursor. Some terminals do not support this, but it is
    # purely cosmetic so. It is safe to continue if it is not supported.
    try:
        curses.curs_set(0)
    except curses.error:
        pass

    curses_menu = CursesMenu(menu, stdscreen)
    return curses_menu.display()


def _interactive_curses_menu(menu):
    result = curses.wrapper(lambda stdscreen: _curses_menu_entry_point(menu, stdscreen))
    if result == "CANCEL":
        _info("Aborting submission.")
        sys.exit(0)
    return result


def _interactive_configure(global_config):
    if os.path.exists(USER_CONFIG_ROOT) and not os.path.isdir(USER_CONFIG_ROOT):
        _fail("Not a directory: %s" % USER_CONFIG_ROOT)

    # make config root if not exists
    try:
        os.mkdir(USER_CONFIG_ROOT)
    except OSError as e:
        if e.errno != errno.EEXIST:
            raise

    # load existing config
    user_config = _read_config(USER_CONFIG_ROOT)
    remote = None

    # set remote if there is no global remote
    if not global_config.has_option(CONFIG_SECTION, "remote"):
        url = None
        while (
            not remote
            or remote == CONFIG_SECTION
            or remote.lower() == "default"  # Python throws error for this
        ):
            if remote is not None:
                _warn("Invalid remote name.")
            remote = (
                input("Default remote name (%s): " % DEFAULT_REMOTE) or DEFAULT_REMOTE
            )
        while not url:
            url = input("Enter submit URL: ")
        if not user_config.has_section(CONFIG_SECTION):
            user_config.add_section(CONFIG_SECTION)
        user_config.set(CONFIG_SECTION, "remote", remote)
        if not user_config.has_section(remote):
            user_config.add_section(remote)
        user_config.set(remote, "url", url)

    # set a username if there is not one
    has_username = global_config.has_option(CONFIG_SECTION, "username")
    use_system_username = global_config.has_option(
        CONFIG_SECTION, "use_system_username"
    ) and global_config.getboolean(CONFIG_SECTION, "use_system_username")
    if not (has_username or use_system_username):
        username = input("Default username (leave blank for none): ")
        if username:
            user_config.set(remote, "username", username)

    path = os.path.join(USER_CONFIG_ROOT, "config")
    with open(path, "w") as fp:
        user_config.write(fp)


def _build_archive(files):
    """Build an archive from a list of files.

    Returns the file system path to the built archive.
    """
    name = tempfile.mktemp(prefix="handin.", suffix=".tar.gz")
    tar = tarfile.open(name, mode="w:gz")
    for f in files:
        _info("Adding %s" % f)
        tar.add(f)
    tar.close()
    return name


def _get_parser():
    parser = OptionParser(
        usage=USAGE,
        version=FORK,
        description=DESCRIPTION,
        epilog=EPILOG,
        prog=PROG,
    )
    parser.add_option("-r", "--remote", default=None, help="Remote name to submit to")
    parser.add_option("-u", "--user", default=None, help="Username to log in with")
    parser.add_option("-b", "--bucket", default=None, help="Bucket identifier")
    parser.add_option(
        "-q", "--quiet", default=0, action="count", help="Show less output"
    )
    parser.add_option(
        "-v", "--verbose", default=0, action="count", help="Show more output"
    )
    parser.add_option(
        "-C",
        "--cabundle",
        default=None,
        help="Certificate authority bundle for use with SSL",
    )
    parser.add_option(
        "--configure", action="store_true", help="Run interactive configuration."
    )
    parser.add_option(
        "--remotes", action="store_true", help="List configured remote names."
    )
    return parser


if __name__ == "__main__":
    main()
