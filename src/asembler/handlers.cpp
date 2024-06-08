#include "../../inc/asembler/singlePassAssembler.hpp"

const DirectiveHandler SinglePassAssembler::directiveHandlers[] = {
  &SinglePassAssembler::handleGlobalDirective,
  &SinglePassAssembler::handleExternDirective,
  &SinglePassAssembler::handleSectionDirective,
  &SinglePassAssembler::handleWordDirective,
  &SinglePassAssembler::handleSkipDirective,
  &SinglePassAssembler::handleAsciiDirective,
  &SinglePassAssembler::handleEquDirective,
  &SinglePassAssembler::handleEndDirective
};


const InstructionHandler SinglePassAssembler::instructionHandlers[] = {
  &SinglePassAssembler::handleHaltInstruction,
  &SinglePassAssembler::handleIntInstruction,
  &SinglePassAssembler::handleIretInstruction,
  &SinglePassAssembler::handleCallInstruction,
  &SinglePassAssembler::handleRetInstruction,
  &SinglePassAssembler::handleJmpInstruction,
  &SinglePassAssembler::handleBeqInstruction,
  &SinglePassAssembler::handleBneInstruction,
  &SinglePassAssembler::handleBgtInstruction,
  &SinglePassAssembler::handlePushInstruction,
  &SinglePassAssembler::handlePopInstruction,
  &SinglePassAssembler::handleXchgInstruction,
  &SinglePassAssembler::handleAddInstruction,
  &SinglePassAssembler::handleSubInstruction,
  &SinglePassAssembler::handleMulInstruction,
  &SinglePassAssembler::handleDivInstruction,
  &SinglePassAssembler::handleNotInstruction,
  &SinglePassAssembler::handleAndInstruction,
  &SinglePassAssembler::handleOrInstruction,
  &SinglePassAssembler::handleXorInstruction,
  &SinglePassAssembler::handleShlInstruction,
  &SinglePassAssembler::handleShrInstruction,
  &SinglePassAssembler::handleLdInstruction,
  &SinglePassAssembler::handleStInstruction,
  &SinglePassAssembler::handleCsrrdInstruction,
  &SinglePassAssembler::handleCsrwrInstruction,
};
