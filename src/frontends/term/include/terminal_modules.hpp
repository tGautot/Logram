#ifndef TERMINAL_MODULES_HPP
#define TERMINAL_MODULES_HPP

#include "logram_terminal_module.hpp"

class CursorMoveModule : public LogramTerminalModule {
  public:
  void registerUserInputMapping(LogramTerminal&) override;
  void registerUserActionCallback(LogramTerminal&) override;
  void registerCommandCallback(LogramTerminal&) override;
};
class ArrowsModule : public LogramTerminalModule {
  public:
  void registerUserInputMapping(LogramTerminal&) override;
  void registerUserActionCallback(LogramTerminal&) override;
  void registerCommandCallback(LogramTerminal&) override;
};
class WasdModule : public LogramTerminalModule {
  public:
  void registerUserInputMapping(LogramTerminal&) override;
  void registerUserActionCallback(LogramTerminal&) override;
  void registerCommandCallback(LogramTerminal&) override;
};
class TextSearchModule : public LogramTerminalModule {
  public:
  void registerUserInputMapping(LogramTerminal&) override;
  void registerUserActionCallback(LogramTerminal&) override;
  void registerCommandCallback(LogramTerminal&) override;
};
class VimQuitModule : public LogramTerminalModule {
  public:
  void registerUserInputMapping(LogramTerminal&) override;
  void registerUserActionCallback(LogramTerminal&) override;
  void registerCommandCallback(LogramTerminal&) override;
};


class FilterManagementModule : public LogramTerminalModule {
  public:
  void registerUserInputMapping(LogramTerminal&) override;
  void registerUserActionCallback(LogramTerminal&) override;
  void registerCommandCallback(LogramTerminal&) override;
};

class VimMotionsModule : public LogramTerminalModule {
  public:
  void registerUserInputMapping(LogramTerminal&) override;
  void registerUserActionCallback(LogramTerminal&) override;
  void registerCommandCallback(LogramTerminal&) override;
};

class ConfigManagerModule : public LogramTerminalModule {
  public:
  void registerUserInputMapping(LogramTerminal&) override;
  void registerUserActionCallback(LogramTerminal&) override;
  void registerCommandCallback(LogramTerminal&) override;
};

class HideLatestErrorModule : public LogramTerminalModule {
  public:
  void registerUserInputMapping(LogramTerminal&) override;
  void registerUserActionCallback(LogramTerminal&) override;
  void registerCommandCallback(LogramTerminal&) override;
};

#endif