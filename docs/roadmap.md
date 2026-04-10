# Roadmap

Logram is currently in Beta. Below are laid out and discussed the minimum features I would like to have before anouncing an official 1.0 release. Besides all those, there are of course many improvements to the current code that need to be brought, this is just an overview of _additions_ I would like make. Any contributions to the project (towards those features or any other improvements) are naturally welcome, and if you wish to discuss any part of the project, please reach out! 

## Saving/Reloading the state

Upon exit of the application, the state of the terminal for the opened file should ideally be saved. This data could then be used upon re-opening the same file to make it feel like the user never left the application at all. As for where to store this data, either the current configuration file (`.logram`) could be used/extended, or some lightweight files could be created next to each open file (e.g. next `mylog.out` logram would create `.mylog.out.lgm`)

## Adding a {DATE:...} field

This date field could take a datetime format string (format of the format string to be defined), to more easily define the format for logs with long, complex date strings. It should be possible to create filters checking against the whole date (e.g. `date GREATER 2016-16-16 16h16m16s`). But also, this date field would automatically define mutliple different fields (year, day, hour, millis, ...) that the user could filter on specifically. On the longer term this would also help standardise time evaluation across different files (since all of them would define the same time fields), making it easier to create Multi-File viewers sorted on timestamps

## Formalize and commit to a stable API for modules to use

I am quite a supporter of the "make everything public" idea. This, unfortunately, does not bode well with a modular program. Between any 2 different versions, internals are bound to change, making modules compiled for one version of the software incompatible with almost any other version. While having to recompile modules upon updates of the software is acceptable (modules should be small, so quick to compile), having to always update them to SUCCESSFULY compile or make sure their logic is always correct is absolutely NOT ok.

Because of this, before 1.0, there should be a decision and implementation on a stable API the modules can use. Modules interacting with Logram exclusively through that API should have the certainty that they won't break between version; breaking anything _outside_ that API is fair game though.

## Compilation withouth tests

Since the only external dependency of Logram is Catch2, the test framework, compilation of the tests sources will automatically pull the dependency through CMake's FetchContent. Thus, compilation of the test suites should be opt-in rather than opt-out.

## Runtime enabling/disabling of modules

Everything is in the title of this one. I think simply mandating all modules libs to have a pre-specified entry point that can be found by using dlopen/dlsym should be good enough.

## Improvment to tests

The tests need to be improved, some features are not covered at all, currently the tests need to be ran exactly from the `Logram/build/bin` folder since some paths to sample tests files are hardcoded, the "production" config file `.logram` is used (meaning that if someone updates their `.logram` and THEN runs the tests, they will most likely fail)... 
Plenty of improvements possible!

## This usage guide

I know this usage guide can be improved in MANY ways, but you, dear reader, are the better judge of how this can be accomplished. I will be waiting for your suggestions!
