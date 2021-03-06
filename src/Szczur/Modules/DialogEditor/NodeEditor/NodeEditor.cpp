#include "NodeEditor.hpp"

#include <iostream>
#include <fstream>

#include <Shared/Math2D.h>
#include <Shared/Interop.h>

#include "Szczur/Config.hpp"
#include "Szczur/Utility/Logger.hpp"
#include "Szczur/Utility/MsgBox.hpp"

#include "../DialogEditor.hpp"

namespace rat
{

NodeEditor::NodeEditor(DialogEditor* dialogEditor)
	: _dialogEditor(dialogEditor)
{
	_context = ed::CreateEditor(nullptr);
	ed::SetCurrentEditor(_context);

	_nodeManager = std::make_unique<NodeManager>();

	createNew();

	ed::NavigateToContent();

	_parts = &_dialogEditor->_dlgEditor.getContainer();

	loadIcons();
}

NodeEditor::~NodeEditor()
{
	ed::DestroyEditor(_context);
}

void NodeEditor::createNew()
{
	reset();

	auto start = _nodeManager->createNode("Start", Node::Start);
	start->createPin(ed::PinKind::Output);

	auto end = _nodeManager->createNode("End", Node::End);
	end->createPin(ed::PinKind::Input);

	ed::SetNodePosition(start->Id, ImVec2(100.f, 100.f));
	ed::SetNodePosition(end->Id, ImVec2(300, 100.f));
}

void NodeEditor::reset()
{
	ed::DestroyEditor(_context);

	_context = ed::CreateEditor(nullptr);
	ed::SetCurrentEditor(_context);

	_nodeManager->reset();

	_optionConfigWindow = false;
	_currentOption = nullptr;

	_optionFunctionConfigWindow = false;
	_currentFunctionOption = nullptr;
}

void NodeEditor::save(const std::string& fileName, FileFormat saveFormat)
{
	if (saveFormat == FileFormat::Lua)
	{
		LOG_INFO("Save format: Lua");
		LOG_INFO("Saving code to '", fileName, "'...");

		backupLuaFunctions();
		std::ofstream file(fileName, std::ios::trunc);

		if (file.good())
		{
			auto code = generateCode();
			file << code;
			file.close();

			LOG_INFO("Saved!");
		}
		else
		{
			LOG_ERROR("Cannot save code to file!");
			MsgBox::show(strerror(errno), "Cannot save", MsgBox::Icon::Error);
		}
	}
	else if (saveFormat == FileFormat::Json)
	{
		LOG_INFO("Save format: Json");
		LOG_INFO("Saving to '", fileName, "'...");

		std::ofstream file(fileName, std::ios::trunc);

		if (file.good())
		{
			Json j;

			_nodeManager->write(j);

			file << j;

			file.close();

			LOG_INFO("Saved!");
		}
		else
		{
			LOG_ERROR("Cannot save to file!");
			MsgBox::show(strerror(errno), "Cannot save", MsgBox::Icon::Error);
		}
	}
}

void NodeEditor::load(const std::string& fileName, FileFormat loadFormat)
{
	if (loadFormat == FileFormat::Lua)
	{
		// @todo
	}
	else if (loadFormat == FileFormat::Json)
	{
		LOG_INFO("Load format: Json");
		LOG_INFO("Loading from '", fileName, "'...");

		std::ifstream file(fileName);

		if (file.good())
		{
			reset();

			Json j;
			file >> j;
			file.close();

			if (_nodeManager->read(j))
			{
				auto& nodes = _nodeManager->getNodes();

				for (auto& node : nodes)
				{
					for (auto& output : node->Outputs)
					{
						if (output->Kind == ed::PinKind::Output)
						{
							auto id = output->OptionTarget.Id;

							if (id.Major >= 0 && id.Major < _parts->size())
							{
								auto& major = _parts->at(id.Major);

								if (id.Minor >= 0 && id.Minor < major.size())
								{
									output->OptionTarget.Ptr = major.at(id.Minor).get();
									output->OptionTarget.WeakPtr = major.at(id.Minor);
								}
							}
						}
					}
				}

				LOG_INFO("Loaded!");
			}
			else
			{
				createNew();
			}
		}
		else
		{
			LOG_ERROR("Cannot open file!");
			MsgBox::show(strerror(errno), "Cannot open file", MsgBox::Icon::Error);
		}

	}
}

std::string NodeEditor::generateCode()
{
	LOG_INFO("Generating code...");

	std::vector<std::string> codeSegment;

	codeSegment.push_back("local dialog = Dialog.load(\"" + _dialogEditor->_projectPath + "/dialog\")\n\n");

	for (auto& character : _dialogEditor->_characters)
	{
		if (!character.name.empty() && !character.imagePath.empty())
		{
			codeSegment.push_back("dialog:bindCharacter(\"" + character.name + "\", \"" + character.imagePath + "\")");
		}
	}

	codeSegment.push_back(std::string());

	bool firstOptions = true;

	for (int i = 2; i < _nodeManager->getNodes().size(); i++)
	{
		auto& node = _nodeManager->getNodes()[i];

		if (node->Type == Node::Options)
		{
			LOG_INFO("Generating ", node->Name.c_str(), "...");

			///// Runners
			std::string code;
			
			code += "-- " + node->Name + "\n\n";

			if (firstOptions)
			{
				code += "local ";
				firstOptions = false;
			}

			code += "options = dialog:newOptions()\n";

			auto& in = node->Inputs.front();

			std::vector<std::pair<int, int>> runners;

			for (auto& link : _nodeManager->getLinks())
			{
				if (link->EndPinId == in->Id)
				{
					auto pin = _nodeManager->findPin(link->StartPinId);

					auto optionTargetPtr = pin->OptionTarget.Ptr;

					if (pin && optionTargetPtr)
					{
						if (optionTargetPtr->id < 0)
							continue;

						bool skip = false;

						// check if runner is already in
						for (auto runner : runners)
						{
							auto major = runner.first;
							auto minor = runner.second;

							if (optionTargetPtr->id == major && optionTargetPtr->minorId == minor)
							{
								skip = true;
								break;
							}
						}

						if (skip)
							continue;

						code += "options:addRunner(" + std::to_string(optionTargetPtr->id) + ", " + std::to_string(optionTargetPtr->minorId) + ")\n";

						runners.push_back(std::make_pair(optionTargetPtr->id, optionTargetPtr->minorId));
					}
				}
			}

			code += "\n\n";

			////// Options
			for (auto& out : node->Outputs)
			{
				code += "options.add = {\n";

				// target
				if (out->OptionTarget.Ptr)
				{
					code += "\tmajorTarget = " + std::to_string(out->OptionTarget.Ptr->id) + ";\n";
					code += "\tminorTarget = " + std::to_string(out->OptionTarget.Ptr->minorId) + ";\n";
				}

				// color
				if (out->Colorful)
				{
					code += "\tcolor = {"	+ std::to_string(static_cast<int>(out->Color.r * 255)) + ", " 
											+ std::to_string(static_cast<int>(out->Color.g * 255)) + ", "
											+ std::to_string(static_cast<int>(out->Color.b * 255)) + ", " 
											+ std::to_string(static_cast<int>(out->Color.a * 255)) + "};\n";
				}

				if (out->Icon)
				{
					code += "\ticonId = " + std::to_string(out->IconId) + ";\n";
				}

				// skip
				if (out->SkipOptions)
				{
					code += "\tskip = true;\n";
				}

				// condition
				if (out->ConditionFunc)
				{
					code += "\n\t--b:c(" + std::to_string(out->Id) + "): " + out->ConditionFuncName + "\n";

					if (out->ConditionFuncCode.empty())
						code += "\tcondition = function()\n\t\t\n\t\tend;\n";
					else
						code += out->ConditionFuncCode;

					code += "\t--e:c(" + std::to_string(out->Id) + ")\n\n";
				}

				// action
				if (out->ActionFunc)
				{
					code += "\n\t--b:a(" + std::to_string(out->Id) + "): " + out->ActionFuncName + "\n";
					
					if (out->ActionFuncCode.empty())
						code += "\taction = function()\n\t\t\n\t\tend;\n";
					else
						code += out->ActionFuncCode;

					code += "\t--e:a(" + std::to_string(out->Id) + ")\n\n";
				}

				// finishing
				for (auto& link : _nodeManager->getLinks())
				{
					if (link->StartPinId == out->Id)
					{
						auto endPin = _nodeManager->findPin(link->EndPinId);

						if (endPin && endPin->Node->Type == Node::End)
						{
							code += "\tfinishing = true;\n";
							break;
						}
					}
				}

				code += "}\n\n";
			}

			codeSegment.push_back(code);
		}
	}

	auto& pinStartNode = _nodeManager->getNodes().front()->Outputs.front();

	LOG_INFO("Finalizing generating code...");

	// start with
	for (auto& link : _nodeManager->getLinks())
	{
		if (pinStartNode->OptionTarget.Ptr && link->StartPinId == pinStartNode->Id)
		{
			std::string code = "dialog:startWith(" + std::to_string(pinStartNode->OptionTarget.Ptr->id) + ", " + std::to_string(pinStartNode->OptionTarget.Ptr->minorId) + ")";

			codeSegment.push_back(code);
		}
	}

	codeSegment.push_back("dialog:play()");

	std::string finalCode;

	for (auto& code : codeSegment)
	{
		finalCode += code + "\n";
	}

	return finalCode;
}

void NodeEditor::backupLuaFunctions()
{
	LOG_INFO("Backuping lua functions from ", _dialogEditor->_projectPath + "/dialog.lua", "...");

	std::ifstream file(_dialogEditor->_projectPath + "/dialog.lua");
	
	if (file.bad())
		return;

	std::string code((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	file.close();

	size_t lastIndex = 0;

	for (auto& node : _nodeManager->getNodes())
	{
		for (auto& out : node->Outputs)
		{
			// condition
			if (out->ConditionFunc)
			{
				out->ConditionFuncCode = getLuaFunction(out.get(), FunctionType::Condition, code, &lastIndex);
			}

			// action
			if (out->ActionFunc)
			{
				out->ActionFuncCode = getLuaFunction(out.get(), FunctionType::Action, code, &lastIndex);
			}
		}
	}
}

std::string NodeEditor::getLuaFunction(NodePin* pin, FunctionType functionType, const std::string& code, size_t* lastIndex)
{
	if (functionType == FunctionType::None)
		return std::string();

	std::string _code;

	if (code.empty())
	{
		std::ifstream file(_dialogEditor->_projectPath + "/dialog.lua");

		if (file.bad())
			return std::string();

		_code = std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		file.close();
	}
	else
	{
		_code = code;
	}

	size_t _lastIndex = lastIndex == nullptr ? 0 : *lastIndex;

	std::string beginString = functionType == FunctionType::Action ? "--b:a(" : "--b:c(";
	std::string endString = functionType == FunctionType::Action ? "--e:a(" : "--e:c(";

	size_t start = _code.find(beginString + std::to_string(pin->Id) + "):", _lastIndex);

	if (start != std::string::npos)
	{
		size_t codeStart = _code.find("\n", start) + 1; // +1 - remove new line
		size_t end = _code.find(endString + std::to_string(pin->Id) + ")", codeStart) - 1; // -1 - remove tab

		if (end == std::string::npos)
			return std::string();

		std::string func = _code.substr(codeStart, end - codeStart);
		//LOG_INFO("(", codeStart, ", ", end, "): Condition:\n", func);

		if (lastIndex != nullptr)
			*lastIndex = end;

		return func;
	}

	return std::string();
}

void NodeEditor::showTooltip(const std::string& message, sf::Sprite* sprite, const std::string& spriteName)
{
	ImGui::SetNextWindowPos(ed::CanvasToScreen(ImGui::GetMousePos()) + ImVec2(16, 8));

	ImGui::BeginTooltip();
	ImGui::Text(message.c_str());

	if (sprite)
	{
		ImGui::Text(spriteName.c_str());
		ImGui::SameLine();
		ImGui::ImageButton(*sprite, 0);
	}

	ImGui::EndTooltip();
}

int NodeEditor::generateNodeNameId()
{
	std::vector<int> ids = { 0 };

	for (auto& node : _nodeManager->getNodes())
	{
		if (node->Type == Node::NodeType::Options)
			ids.push_back(node->NameId);
	}

	//for (auto id : ids)
	//{
	//	LOG_INFO("ID: ", id);
	//}

	std::sort(ids.begin(), ids.end());

	int i = 0;
	for (i = 0; i < ids.size() - 1; ++i)
	{
		int current = ids[i];
		int next = ids[i + 1];

		if (next - current > 1)
		{
			return current + 1;
		}
	}

	if (i == ids.size() - 1)
	{
		return ids.back() + 1;
	}
	
	return -1;
}

void NodeEditor::update()
{
	bool renamePopup = false;

	ed::Begin("Node Editor");
	{
		auto cursorTopLeft = ImGui::GetCursorScreenPos();

		// draw nodes
		for (auto& node : _nodeManager->getNodes())
		{
			ed::BeginNode(node->Id);
			{
				ImGui::Text(node->Name.c_str());

				if (node->Type == Node::Options)
				{
					ImGui::SameLine();

					if (ImGui::Button(("Rename##" + std::to_string(node->Id)).c_str()))
					{
						strcpy(_renameBuffer, node->Name.c_str());
						renamePopup = true;
						_contextId = node->Id;
					}
				}

				ImGui::BeginGroup();

				// draw inputs
				for (auto& input : node->Inputs)
				{
					ed::PushStyleVar(ed::StyleVar_PivotAlignment, ImVec2(0, 0.5f));
					ed::PushStyleVar(ed::StyleVar_PivotSize, ImVec2(0, 0));

					ed::BeginPin(input->Id, ed::PinKind::Input);
					
					drawIcon(_nodeManager->isPinLinked(input->Id));

					ImGui::Dummy(ImVec2(24, 24));
					ImGui::SameLine();

					ImGui::BeginGroup();
					ImGui::Dummy(ImVec2(1.f, 0.f));
					ImGui::Text(input->Node->Type == Node::NodeType::Options ? "Trigger" : "End");
					ImGui::EndGroup();


					ed::EndPin();

					ed::PopStyleVar(2);
				}

				ImGui::EndGroup();

				ImGui::SameLine();

				ImGui::BeginGroup();

				// draw outputs
				for (auto& output : node->Outputs)
				{
					if (output->OptionTarget.WeakPtr.expired())
						output->OptionTarget.Ptr = nullptr;
						
					ed::PushStyleVar(ed::StyleVar_PivotAlignment, ImVec2(1.0f, 0.5f));
					ed::PushStyleVar(ed::StyleVar_PivotSize, ImVec2(0, 0));

					ed::BeginPin(output->Id, ed::PinKind::Output);

					if (node->Type == Node::Options || node->Type == Node::Start)
					{
						ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));
						

						std::string label = (output->OptionTarget.Ptr == nullptr ? "SET" : std::to_string(output->OptionTarget.Ptr->id) + ":" + std::to_string(output->OptionTarget.Ptr->minorId)) + "##" + std::to_string(output->Id);

						if (ImGui::Button(label.c_str()))
						{
							ImGui::SetWindowFocus("Option config");

							_currentOption = output.get();
							_optionConfigWindow = true;
						}

						if (output->OptionTarget.Ptr)
						{
							if (ImGui::IsItemHovered())
							{
								std::string str;
								str = "Major: " + output->OptionTarget.Ptr->majorFullName;
								str += "\nMinor: " + output->OptionTarget.Ptr->minorFullName;

								auto tex = output->IconId > -1 ? _icons.at(output->IconId).get() : nullptr;

								showTooltip(str, tex, "Icon:");
							}
						}

						ImGui::PopStyleVar();
						ImGui::SameLine();
					}

					ImGui::BeginGroup();

					if (output->OptionTarget.Ptr == nullptr)
					{
						ImGui::Text("Not selected");
					}
					else
					{
						ImGui::PushID(output->Id);

						if (output->Colorful)
						{
							ImGui::ColorButton("", ImVec4(output->Color.r, output->Color.g, output->Color.b, output->Color.a), 0, ImVec2(8, 24));
							ImGui::SameLine();
						}

						ImGui::Text(output->OptionTarget.Ptr->label.c_str());

						if (output->ActionFunc)
						{
							ImGui::SameLine();

							ImGui::PushStyleColor(ImGuiCol_Button, ImColor(183, 28, 28).Value);
							
							if (ImGui::Button("!", ImVec2(24, 24)))
							{
								_optionFunctionConfigWindow = true;
								_currentFunctionOption = output.get();
								ImGui::SetWindowFocus("Option function");
								_functionType = FunctionType::Action;
								output->ActionFuncCode = getLuaFunction(output.get(), FunctionType::Action);
							}

							ImGui::PopStyleColor();

							if (ImGui::IsItemHovered())
							{
								showTooltip(output->ActionFuncName);
							}
						}

						if (output->ConditionFunc)
						{
							ImGui::SameLine();
					
							ImGui::PushStyleColor(ImGuiCol_Button, ImColor(63, 81, 181).Value);
							
							if (ImGui::Button("?", ImVec2(24, 24)))
							{
								_optionFunctionConfigWindow = true;
								_currentFunctionOption = output.get();
								ImGui::SetWindowFocus("Option function");
								_functionType = FunctionType::Condition;
								output->ConditionFuncCode = getLuaFunction(output.get(), FunctionType::Condition);
							}

							ImGui::PopStyleColor();

							if (ImGui::IsItemHovered())
							{
								showTooltip(output->ConditionFuncName);
							}
						}

						ImGui::PopID();
					}

					ImGui::EndGroup();

					ImGui::SameLine();

					if (output->LinkToSameNode)
						drawIcon(_nodeManager->isPinLinked(output->Id), ImColor(33, 150, 243));
					else
						drawIcon(_nodeManager->isPinLinked(output->Id));

					ImGui::Dummy(ImVec2(24, 24));

					ed::EndPin();

					ed::PopStyleVar(2);
				}

				if (node->Type == Node::Options)
				{
					std::string label = "Add option##" + std::to_string(node->Id);

					if (ImGui::Button(label.c_str()))
					{
						[[maybe_unused]] auto pin = node->createPin(ed::PinKind::Output);
					}
				}

				ImGui::EndGroup();

			}
			ed::EndNode();
		}

		// draw links
		for (auto& link : _nodeManager->getLinks())
		{
			ed::Link(link->Id, link->StartPinId, link->EndPinId, link->Color, 2.f);
		}

		// create link
		if (ed::BeginCreate(ImColor(255, 255, 255), 2.0f))
		{
			int startPinId = 0, endPinId = 0;

			if (ed::QueryNewLink(&startPinId, &endPinId))
			{
				auto startPin = _nodeManager->findPin(startPinId);
				auto endPin = _nodeManager->findPin(endPinId);

				_newLinkPin = startPin ? startPin : endPin;

				if (startPin->Kind == ed::PinKind::Input)
				{
					std::swap(startPin, endPin);
					std::swap(startPinId, endPinId);
				}

				if (startPin && endPin)
				{
					if (endPin == startPin)
					{
						ed::RejectNewItem(ImColor(255, 0, 0), 2.0f);
					}
					else if (endPin->Kind == startPin->Kind)
					{
						ed::RejectNewItem(ImColor(255, 0, 0), 2.0f);
					}
					else if (startPin->Node->Type == Node::Start && endPin->Node->Type == Node::End)
					{
						ed::RejectNewItem(ImColor(255, 0, 0), 1.0f);
					}
					else if (_nodeManager->isPinLinked(startPinId))
					{
						ed::RejectNewItem(ImColor(255, 0, 0), 1.0f);
					}
					else
					{
						if (ed::AcceptNewItem(ImColor(128, 255, 128), 4.0f))
						{
							auto link = _nodeManager->createLink(startPinId, endPinId);

							if (startPin->Node == endPin->Node)
							{
								link->SameNode = true;
								link->Color.Value.w = 0.f;
								startPin->LinkToSameNode = true;
							}
							else
							{
								link->SameNode = false;
								startPin->LinkToSameNode = false;
							}
						}
					}
				}
			}
		}

		ed::EndCreate();

		if (ed::BeginDelete())
		{
			// delete links
			int linkId = 0;
			while (ed::QueryDeletedLink(&linkId))
			{
				if (ed::AcceptDeletedItem())
				{
					auto link = _nodeManager->findLink(linkId);

					if (link)
					{
						auto startPin = _nodeManager->findPin(link->StartPinId);
						
						if (startPin)
						{
							startPin->LinkToSameNode = false;
						}
					}

					_nodeManager->removeLink(linkId);
				}
			}

			// delete nodes
			int nodeId = 0;
			while (ed::QueryDeletedNode(&nodeId))
			{
				auto node = _nodeManager->findNode(nodeId);

				if (node != nullptr && node->Type == Node::Options && ed::AcceptDeletedItem())
				{
					_nodeManager->removeNode(nodeId);
				}
				else
				{
					ed::RejectDeletedItem();
				}
			}

			ed::EndDelete();
		}

		ImGui::SetCursorScreenPos(cursorTopLeft);
	}
	ed::End();

	if (ed::ShowNodeContextMenu(&_contextId))
		ImGui::OpenPopup("Node Context Menu");
	else if (ed::ShowLinkContextMenu(&_contextId))
		ImGui::OpenPopup("Link Context Menu");
	else if (ed::ShowPinContextMenu(&_contextId))
		ImGui::OpenPopup("Pin Context Menu");
	else if (ed::ShowBackgroundContextMenu())
	{
		if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows))
		{
			ImGui::OpenPopup("Background Context Menu");
		}
	}

	if (renamePopup)
	{
		renamePopup = false;
		ImGui::OpenPopup("Rename Node Popup");
	}

	showPopups();
	showOptionConfig();
	showOptionFunctionConfig();
}

void NodeEditor::loadIcons()
{
	std::string iconsPath = "Assets/Dialog/Config/icons.png";


	LOG_INFO("Icons path: ", iconsPath);

	_iconsTex = std::make_unique<sf::Texture>();

	if (!_iconsTex->loadFromFile(iconsPath))
	{
		LOG_INFO("Cannot load icons!");
		_iconsTex.reset();
		return;
	}

	

	sf::Vector2i iconSize = sf::Vector2i(_iconsTex->getSize().y, _iconsTex->getSize().y);

	LOG_INFO("Icon size: ", iconSize.x, "x", iconSize.y);

	auto iconNumber = _iconsTex->getSize().x / iconSize.x;

	LOG_INFO("Icons number: ", iconNumber);

	for (int i = 0; i < iconNumber; i++)
	{
		auto icon = std::make_unique<sf::Sprite>();
		icon->setTexture(*_iconsTex);
		icon->setTextureRect(sf::IntRect({ iconSize.x * i, 0 }, iconSize));
		icon->setScale(0.5f, 0.5f);

		_icons.push_back(std::move(icon));
	}

}

void NodeEditor::showPopups()
{
	if (ImGui::BeginPopup("Background Context Menu"))
	{
		if (ImGui::MenuItem("Create options"))
		{
			auto nameId = generateNodeNameId();

			auto node = _nodeManager->createNode("Options ");
			node->Name += std::to_string(nameId);
			node->NameId = nameId;
			node->createPin(ed::PinKind::Input);

			ed::SetNodePosition(node->Id, ed::ScreenToCanvas(ImGui::GetMousePos()));
		}
		
		ImGui::EndPopup();
	}

	if (ImGui::BeginPopup("Node Context Menu"))
	{
		if (ImGui::MenuItem("Delete"))
			ed::DeleteNode(_contextId);

		ImGui::EndPopup();
	}

	if (ImGui::BeginPopup("Link Context Menu"))
	{
		if (ImGui::MenuItem("Delete"))
			ed::DeleteLink(_contextId);

		ImGui::EndPopup();
	}

	if (ImGui::BeginPopup("Pin Context Menu"))
	{
		auto pin = _nodeManager->findPin(_contextId);

		if (ImGui::MenuItem("Disconnect"))
		{
			auto& links = _nodeManager->getLinks();

			for (int i = 0; i < links.size(); i++)
			{
				auto& link = links[i];

				if (link->StartPinId == pin->Id || link->EndPinId == pin->Id)
				{
					auto startPin = _nodeManager->findPin(link->StartPinId);

					if (startPin)
					{
						startPin->LinkToSameNode = false;
					}

					ed::DeleteLink(link->Id);
				}
			}
		}


		if (pin->Kind == ed::PinKind::Output && pin->Node->Type == Node::Options)
		{
			if (ImGui::MenuItem("Delete"))
			{
				if (pin->Kind == ed::PinKind::Output)
				{
					// remove links with this pin
					auto& links = _nodeManager->getLinks();

					for (int i = 0; i < links.size(); i++)
					{
						auto& link = links[i];

						if (link->StartPinId == pin->Id)
						{
							_nodeManager->removeLink(link->Id);

							i--;
						}
					}

					pin->Node->removePin(pin);
				}
			}
		}
		ImGui::EndPopup();
	}

	if (ImGui::BeginPopupModal("Rename Node Popup", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		if (_contextId < 1)
			ImGui::CloseCurrentPopup();

		ImGui::Text("Rename node");
		ImGui::Separator();

		ImGui::InputText("Node name", _renameBuffer, 256);

		if (ImGui::Button("OK", ImVec2(120, 0))) 
		{
			auto node = _nodeManager->findNode(_contextId);

			node->Name = _renameBuffer;

			ImGui::CloseCurrentPopup(); 
		}

		ImGui::SameLine();

		if (ImGui::Button("Cancel", ImVec2(120, 0))) 
		{ 
			ImGui::CloseCurrentPopup(); 
		}

		ImGui::EndPopup();
	}
}

void NodeEditor::showOptionConfig()
{
	if (_optionConfigWindow)
	{
		if (ImGui::Begin("Option config", &_optionConfigWindow, ImGuiWindowFlags_AlwaysAutoResize))
		{
			if (_currentOption)
			{
				ImGui::Text("Node: %s", _currentOption->Node->Name.c_str());

				ImGui::Separator();

				ImGui::Text("Set target dialog");

				ImGui::SameLine();

				// edit
				if (ImGui::Button("Edit"))
				{
					if (_currentOption->OptionTarget.Ptr)
					{
						_dialogEditor->_showDlgEditor = true;

						_dialogEditor->_dlgEditor.setCurrentDialog(static_cast<int>(_currentOption->OptionTarget.Ptr->id), static_cast<int>(_currentOption->OptionTarget.Ptr->minorId));

						ImGui::SetWindowFocus("Dlg Files Editor");
					}
				}

				std::string defaultLabelMajor;
				std::string defaultLabelMinor;

				//LOG_INFO(_currentOption->OptionTarget.Ptr);

				if (_currentOption->OptionTarget.Ptr != nullptr)
				{
					[[maybe_unused]] auto dialog = _currentOption->OptionTarget.Ptr;

					defaultLabelMajor = _currentOption->OptionTarget.Ptr->majorFullName;
					defaultLabelMinor = _currentOption->OptionTarget.Ptr->minorFullName;
				}

				if (ImGui::BeginCombo("Target major", defaultLabelMajor.c_str()))
				{
					for (auto& part : *_parts)
					{
						bool isSelected = (_currentOption->OptionTarget.Id.Major == part.front()->id);

						if (ImGui::Selectable(part.front()->majorFullName.c_str(), isSelected))
						{
							_currentOption->OptionTarget.Ptr = part.front().get();
							_currentOption->OptionTarget.WeakPtr = part.front();
							_currentOption->OptionTarget.Id.Major = part.front()->id;
						}

						if (isSelected)
							ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}

				if (ImGui::BeginCombo("Target minor", defaultLabelMinor.c_str()))
				{
					if (_currentOption->OptionTarget.Ptr)
					{
						for (auto part : _parts->at(_currentOption->OptionTarget.Id.Major))
						{
							bool isSelected = (_currentOption->OptionTarget.Ptr == part.get());

							if (ImGui::Selectable(part->minorFullName.c_str(), isSelected))
							{
								_currentOption->OptionTarget.Ptr = part.get();
								_currentOption->OptionTarget.WeakPtr = part;
								_currentOption->OptionTarget.Id.Minor = part->minorId;
							}

							if (isSelected)
								ImGui::SetItemDefaultFocus();
						}
					}
					ImGui::EndCombo();
				}

				if (_currentOption->Node->Type == Node::Options)
				{
					ImGui::Separator();

					ImGui::Checkbox("Skip options", &_currentOption->SkipOptions);

					if (ImGui::IsItemHovered())
					{
						ImGui::BeginTooltip();
						ImGui::Text("Autoselect this option");
						ImGui::EndTooltip();
					}

					ImGui::Separator();

					ImGui::Checkbox("Condition", &_currentOption->ConditionFunc);

					if (_currentOption->ConditionFunc)
					{
						char buffer[128];

						strcpy(buffer, _currentOption->ConditionFuncName.c_str());

						if (ImGui::InputText("Condition name", buffer, 128))
						{
							_currentOption->ConditionFuncName = buffer;
						}

						ImGui::Separator();
					}

					ImGui::Checkbox("Action", &_currentOption->ActionFunc);

					if (_currentOption->ActionFunc)
					{
						char buffer[128];

						strcpy(buffer, _currentOption->ActionFuncName.c_str());

						if (ImGui::InputText("Action name", buffer, 128))
						{
							_currentOption->ActionFuncName = buffer;
						}
					}

					ImGui::Checkbox("Color", &_currentOption->Colorful);

					if (_currentOption->Colorful)
					{
						ImGui::ColorEdit4("Color", &_currentOption->Color[0]);
					}

					ImGui::Checkbox("Icon", &_currentOption->Icon);

					if (_currentOption->Icon)
					{
						for (int i = 0; i < _icons.size(); ++i)
						{
							bool current = _currentOption->IconId == i;

							if (current)
							{
								ImGui::PushStyleColor(ImGuiCol_Button, sf::Color(0x1B5E20FF));
							}

							ImGui::PushID(i);

							if (ImGui::ImageButton(*_icons[i]))
							{
								_currentOption->IconId = i;
							}

							ImGui::PopID();

							if (current)
							{
								ImGui::PopStyleColor();
							}

							if (i != _icons.size() - 1 && (i + 1) % 5 != 0)
							{
								ImGui::SameLine();
							}
						}

					}
				}
			}
		}
		ImGui::End();
	}
}

void NodeEditor::showOptionFunctionConfig()
{
	if (_optionFunctionConfigWindow && _currentFunctionOption && _functionType != FunctionType::None)
	{
		if (ImGui::Begin("Option function", &_optionFunctionConfigWindow, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text("Node name: %s", _currentFunctionOption->Node->Name.c_str());
			ImGui::Text("Function type: %s", _functionType == FunctionType::Action ? "Action" : "Condition");
			
			ImGui::Separator();

			char nameBuffer[128] = { 0 };

			if (_functionType == FunctionType::Action)
			{
				strcpy(nameBuffer, _currentFunctionOption->ActionFuncName.c_str());

				if (ImGui::InputText("Action name", nameBuffer, 128))
				{
					_currentFunctionOption->ActionFuncName = nameBuffer;
				}
			}
			else if (_functionType == FunctionType::Condition)
			{
				strcpy(nameBuffer, _currentFunctionOption->ConditionFuncName.c_str());

				if (ImGui::InputText("Condition name", nameBuffer, 128))
				{
					_currentFunctionOption->ConditionFuncName = nameBuffer;
				}
			}

			ImGui::Separator();

			ImGui::Text(_functionType == FunctionType::Action ? _currentFunctionOption->ActionFuncCode.c_str() : _currentFunctionOption->ConditionFuncCode.c_str());
		}

		ImGui::End();
	}
}

void NodeEditor::drawIcon(bool filled, ImColor&& color)
{
	auto cursorPos = ImGui::GetCursorScreenPos();
	auto drawList = ImGui::GetWindowDrawList();

	auto a = cursorPos;
	auto b = a + ImVec2(24, 24);

	auto rect = ax::rect(to_point(a), to_point(b));
	const auto outline_scale = rect.w / 24.0f;
	[[maybe_unused]] const auto extra_segments = roundi(2 * outline_scale); // for full circle

	auto innerColor = ImColor(32, 32, 32, 255);

	const auto origin_scale = rect.w / 24.0f;

	const auto offset_x = 1.0f * origin_scale;
	const auto offset_y = 0.0f * origin_scale;
	const auto margin = (filled ? 2.0f : 2.0f) * origin_scale;
	const auto rounding = 0.1f * origin_scale;
	const auto tip_round = 0.7f; // percentage of triangle edge (for tip)
								 //const auto edge_round = 0.7f; // percentage of triangle edge (for corner)
	const auto canvas = ax::rectf(
		rect.x + margin + offset_x,
		rect.y + margin + offset_y,
		rect.w - margin * 2.0f,
		rect.h - margin * 2.0f);

	const auto left = canvas.x + canvas.w            * 0.5f * 0.3f;
	const auto right = canvas.x + canvas.w - canvas.w * 0.5f * 0.3f;
	const auto top = canvas.y + canvas.h            * 0.5f * 0.2f;
	const auto bottom = canvas.y + canvas.h - canvas.h * 0.5f * 0.2f;
	const auto center_y = (top + bottom) * 0.5f;
	//const auto angle = AX_PI * 0.5f * 0.5f * 0.5f;

	const auto tip_top = ImVec2(canvas.x + canvas.w * 0.5f, top);
	const auto tip_right = ImVec2(right, center_y);
	const auto tip_bottom = ImVec2(canvas.x + canvas.w * 0.5f, bottom);

	drawList->PathLineTo(ImVec2(left, top) + ImVec2(0, rounding));
	drawList->PathBezierCurveTo(
		ImVec2(left, top),
		ImVec2(left, top),
		ImVec2(left, top) + ImVec2(rounding, 0));
	drawList->PathLineTo(tip_top);
	drawList->PathLineTo(tip_top + (tip_right - tip_top) * tip_round);
	drawList->PathBezierCurveTo(
		tip_right,
		tip_right,
		tip_bottom + (tip_right - tip_bottom) * tip_round);
	drawList->PathLineTo(tip_bottom);
	drawList->PathLineTo(ImVec2(left, bottom) + ImVec2(rounding, 0));
	drawList->PathBezierCurveTo(
		ImVec2(left, bottom),
		ImVec2(left, bottom),
		ImVec2(left, bottom) - ImVec2(0, rounding));

	if (!filled)
	{
		if (innerColor & 0xFF000000)
			drawList->AddConvexPolyFilled(drawList->_Path.Data, drawList->_Path.Size, innerColor);

		drawList->PathStroke(color, true, 2.0f * outline_scale);
	}
	else
		drawList->PathFillConvex(color);
}

}
