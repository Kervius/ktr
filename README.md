ktr
===

Simple construction/build/make tool

Work in progress and a proof-of-concept. The concept to prove: a build tool is not really a rocket science.

Goals
====

- As simple as possible, but useful.
- Recursive. The build shouldn't depend on current directory it is called. The tool should be able to both ascend and descent the hierarchy of directories.
- Deterministic and explicit. Neither the build tool nor the user should have to guess what's the heck is going on.
- Parallel builds.
- Ability to use an external scripting language for the scripting needs.

To do
=====

- [ ] support for scripting (probably via `boa`)
- [ ] rewrite the kfile parser
- [ ] logging and tracing
