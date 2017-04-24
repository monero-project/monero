# Contributing to Monero

A good way to help is to test, and report bugs. See
[How to Report Bugs Effectively (by Simon Tatham)](http://www.chiark.greenend.org.uk/~sgtatham/bugs.html)
if you want to help that way. Testing is invaluable in making a piece
of software solid and usable.


## General Guidelines

* Comments are encouraged.
* If modifying code for which Doxygen headers exist, that header must be modified to match.
* Tests would be nice to have if you're adding functionality.

Patches are preferably to be sent via a github pull request. If that
can't be done, patches in "git format-patch" format can be sent
(eg, posted to fpaste.org with a long enough timeout and a link
posted to #monero-dev on irc.freenode.net).

Patches should be self contained. A good rule of thumb is to have
one patch per separate issue, feature, or logical change. Also, no
other changes, such as random whitespace changes or reindentation.
Following the code style of the particular chunk of code you're
modifying is encourgaged. Proper squashing should be done (eg, if
you're making a buggy patch, then a later patch to fix the bug,
both patches should be merged).

## Commits and Pull Requests

Commit messages should be sensible. That means a subject line that
describes the patch, with an optional longer body that gives details,
documentation, etc.

When submitting a pull request on github, make sure your branch is
rebased. No merge commits nor stray commits from other people in
your submitted branch, please. You may be asked to rebase if there
are conflicts (even trivially resolvable ones).

PGP signing commits is strongly encouraged. That should explain why
the previous paragraph is here.
