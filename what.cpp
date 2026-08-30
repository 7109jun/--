#include "interpreter.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <functional>
#include <iostream>
#include <regex>
#include <sstream>
#include <stdexcept>

namespace fs = std::filesystem;

namespace hinegx {

namespace {

bool startsWith(const std::string& text, const std::string& prefix) {
    return text.rfind(prefix, 0) == 0;
}

std::string removeTrailingColon(const std::string& text) {
    if (text.empty() || text.back() != ':') {
        throw std::runtime_error("block headers must end with ':'");
    }
    return Interpreter::trim(text.substr(0, text.size() - 1));
}

bool hasWildcard(const std::string& text) {
    return text.find_first_of("*?") != std::string::npos;
}

} // namespace

std::string Value::toString() const {
    if (std::holds_alternative<std::monostate>(data)) return "null";
    if (const auto* number = std::get_if<long long>(&data)) return std::to_string(*number);
    if (const auto* boolean = std::get_if<bool>(&data)) return *boolean ? "true" : "false";
    if (const auto* string = std::get_if<std::string>(&data)) return *string;

    const auto& list = std::get<List>(data);
    std::string result = "[";
    for (std::size_t i = 0; i < list.size(); ++i) {
        if (i > 0) result += ", ";
        result += list[i];
    }
    return result + ']';
}

bool Value::truthy() const {
    if (std::holds_alternative<std::monostate>(data)) return false;
    if (const auto* number = std::get_if<long long>(&data)) return *number != 0;
    if (const auto* boolean = std::get_if<bool>(&data)) return *boolean;
    if (const auto* string = std::get_if<std::string>(&data)) return !string->empty();
    return !std::get<List>(data).empty();
}

int Interpreter::executeFile(const fs::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("cannot open script: " + path.string());
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    return executeText(buffer.str(), path.string());
}

int Interpreter::executeText(const std::string& source, const std::string& origin) {
    origin_ = origin;
    exitRequested_ = false;
    const auto program = parse(source);
    executeBlock(program);
    return 0;
}

int Interpreter::repl() {
    std::cout << "Hinegx 0.1.0 — type 'help' for commands, 'exit' to quit.\n";
    std::string line;
    while (!exitRequested_) {
        std::cout << "Hinegx > ";
        if (!std::getline(std::cin, line)) break;
        if (trim(line).empty()) continue;

        try {
            executeCommand(line, 0);
        } catch (const std::exception& exception) {
            std::cerr << "Error: " << exception.what() << '\n';
        }
    }
    return 0;
}

std::vector<SourceLine> Interpreter::lexLines(const std::string& source) const {
    std::vector<SourceLine> lines;
    std::istringstream stream(source);
    std::string raw;
    std::size_t number = 0;

    while (std::getline(stream, raw)) {
        ++number;
        if (!raw.empty() && raw.back() == '\r') raw.pop_back();

        std::size_t position = 0;
        std::size_t indent = 0;
        while (position < raw.size() && raw[position] == ' ') {
            ++indent;
            ++position;
        }
        if (position < raw.size() && raw[position] == '\t') {
            error(number, "tabs are not allowed; use four spaces per indentation level");
        }

        const auto text = trim(raw.substr(position));
        if (text.empty() || startsWith(text, "#")) continue;
        if (indent % 4 != 0) error(number, "indentation must use multiples of four spaces");
        lines.push_back({number, indent, text});
    }
    return lines;
}

std::vector<Statement> Interpreter::parseBlock(
    const std::vector<SourceLine>& lines,
    std::size_t& index,
    std::size_t indent) const {
    std::vector<Statement> block;

    while (index < lines.size()) {
        const auto& line = lines[index];
        if (line.indent < indent) break;
        if (line.indent > indent) error(line.number, "unexpected indentation");
        if (startsWith(line.text, "else")) {
            if (block.empty() || !startsWith(block.back().text, "if ")) {
                error(line.number, "'else' must follow an 'if' block");
            }
            const auto elseHeader = removeTrailingColon(line.text);
            if (elseHeader != "else") error(line.number, "only 'else:' is supported");
            ++index;
            if (index >= lines.size() || lines[index].indent <= indent) {
                error(line.number, "'else:' requires an indented block");
            }
            block.back().otherwise = parseBlock(lines, index, lines[index].indent);
            continue;
        }

        Statement statement {line.number, line.text, {}, {}};
        ++index;
        if (!statement.text.empty() && statement.text.back() == ':') {
            if (index >= lines.size() || lines[index].indent <= indent) {
                error(statement.line, "block header requires an indented block");
            }
            statement.body = parseBlock(lines, index, lines[index].indent);
        }
        block.push_back(std::move(statement));
    }
    return block;
}

std::vector<Statement> Interpreter::parse(const std::string& source) const {
    const auto lines = lexLines(source);
    if (lines.empty()) return {};
    if (lines.front().indent != 0) error(lines.front().number, "the first statement cannot be indented");
    std::size_t index = 0;
    return parseBlock(lines, index, 0);
}

void Interpreter::executeBlock(const std::vector<Statement>& block) {
    for (const auto& statement : block) {
        if (exitRequested_) return;
        executeStatement(statement);
    }
}

void Interpreter::executeStatement(const Statement& statement) {
    const auto text = trim(statement.text);

    if (startsWith(text, "let ") || startsWith(text, "const ")) {
        const bool isConstant = startsWith(text, "const ");
        const std::size_t keywordLength = isConstant ? 6 : 4;
        const auto assignment = text.find('=');
        if (assignment == std::string::npos) error(statement.line, "variable assignment needs '='");
        const auto name = trim(text.substr(keywordLength, assignment - keywordLength));
        if (name.empty()) error(statement.line, "variable name is missing");
        assign(name, evaluate(text.substr(assignment + 1)), isConstant);
        return;
    }

    if (startsWith(text, "if ")) {
        const auto condition = removeTrailingColon(text).substr(3);
        if (evaluateCondition(condition)) executeBlock(statement.body);
        else executeBlock(statement.otherwise);
        return;
    }

    if (startsWith(text, "for ")) {
        const auto header = removeTrailingColon(text);
        const auto in = header.find(" in ");
        if (in == std::string::npos) error(statement.line, "for syntax is: for item in list:");
        const auto item = trim(header.substr(4, in - 4));
        const auto iterable = evaluate(header.substr(in + 4));
        const auto* list = std::get_if<Value::List>(&iterable.data);
        if (!list) error(statement.line, "a for loop needs a list value");
        for (const auto& value : *list) {
            assign(item, Value{value});
            executeBlock(statement.body);
        }
        return;
    }

    if (startsWith(text, "function ")) {
        const auto header = removeTrailingColon(text);
        const auto open = header.find('(');
        const auto close = header.rfind(')');
        if (open == std::string::npos || close == std::string::npos || close < open) {
            error(statement.line, "function syntax is: function name(parameter):");
        }
        const auto name = trim(header.substr(9, open - 9));
        if (name.empty()) error(statement.line, "function name is missing");
        Function function;
        for (auto parameter : splitArguments(header.substr(open + 1, close - open - 1))) {
            parameter = trim(parameter);
            if (!parameter.empty()) function.parameters.push_back(parameter);
        }
        function.body = statement.body;
        functions_[name] = std::move(function);
        return;
    }

    if (text.back() == ':') error(statement.line, "unknown block statement");
    executeCommand(text, statement.line);
}

void Interpreter::executeCommand(const std::string& command, std::size_t line) {
    const auto expanded = expand(command);
    auto arguments = splitArguments(expanded);
    if (arguments.empty()) return;

    const auto first = lower(arguments[0]);
    if (first == "exit" || first == "quit") {
        exitRequested_ = true;
        return;
    }
    if (first == "version") {
        std::cout << "Hinegx 0.1.0\n";
        return;
    }
    if (first == "help") {
        std::cout << "Commands: files list|count|move|delete, system info, run, print, script run, help, exit\n"
                     "Script statements: let, const, if/else, for, function. Blocks use four spaces and ':'.\n";
        return;
    }
    if (first == "print") {
        const auto message = trim(expanded.substr(arguments[0].size()));
        std::cout << unquote(message) << '\n';
        return;
    }
    if (first == "script" && arguments.size() == 3 && lower(arguments[1]) == "run") {
        executeFile(unquote(arguments[2]));
        return;
    }

    if (first == "system" && arguments.size() >= 2 && lower(arguments[1]) == "info") {
#ifdef _WIN32
        std::cout << "OS: Windows\n";
#else
        std::cout << "OS: non-Windows build (Hinegx targets Windows)\n";
#endif
        std::cout << "Working directory: " << fs::current_path().string() << '\n';
        return;
    }

    if (first == "files") {
        if (arguments.size() < 2) error(line, "files needs an action: list, count, move, or delete");
        const auto action = lower(arguments[1]);

        if (action == "list") {
            const fs::path directory = arguments.size() >= 3 ? unquote(arguments[2]) : fs::current_path();
            if (!fs::exists(directory) || !fs::is_directory(directory)) error(line, "folder not found: " + directory.string());
            std::size_t count = 0;
            for (const auto& entry : fs::directory_iterator(directory)) {
                std::cout << (entry.is_directory() ? "[folder] " : "[file]   ") << entry.path().filename().string() << '\n';
                ++count;
            }
            std::cout << count << " item(s)\n";
            return;
        }

        if (action == "count") {
            if (arguments.size() < 3) error(line, "files count needs a file pattern");
            std::cout << expandFilePattern(unquote(arguments[2])).size() << '\n';
            return;
        }

        if (action == "move") {
            if (arguments.size() < 4) error(line, "files move needs a source pattern and destination folder");
            const auto sources = expandFilePattern(unquote(arguments[2]));
            const fs::path destination = unquote(arguments[3]);
            const bool dryRun = std::find(arguments.begin(), arguments.end(), "--dry-run") != arguments.end();
            const bool renameOnConflict = std::find(arguments.begin(), arguments.end(), "rename") != arguments.end();
            if (sources.empty()) {
                std::cout << "No files matched.\n";
                return;
            }
            if (!dryRun) fs::create_directories(destination);
            for (const auto& source : sources) {
                fs::path target = destination / source.filename();
                if (fs::exists(target) && renameOnConflict) {
                    const auto stem = target.stem().string();
                    const auto extension = target.extension().string();
                    int number = 2;
                    do { target = destination / (stem + " (" + std::to_string(number++) + ")" + extension); } while (fs::exists(target));
                }
                if (fs::exists(target) && !renameOnConflict) {
                    error(line, "destination already has a file named: " + target.filename().string() + ". Use --if-exists rename.");
                }
                if (dryRun) {
                    std::cout << "Would move: " << source.string() << " -> " << target.string() << '\n';
                } else {
                    std::error_code moveError;
                    fs::rename(source, target, moveError);
                    if (moveError) {
                        // rename cannot cross drives on Windows. Copying first keeps the original
                        // file intact if the copy fails.
                        fs::copy_file(source, target, fs::copy_options::none);
                        fs::remove(source);
                    }
                }
            }
            std::cout << (dryRun ? "Previewed " : "Moved ") << sources.size() << " file(s).\n";
            return;
        }

        if (action == "delete") {
            if (arguments.size() < 3) error(line, "files delete needs a file pattern");
            const bool force = std::find(arguments.begin(), arguments.end(), "--force") != arguments.end();
            const bool dryRun = std::find(arguments.begin(), arguments.end(), "--dry-run") != arguments.end();
            if (!force && !dryRun) error(line, "files delete requires --dry-run or --force");
            const auto sources = expandFilePattern(unquote(arguments[2]));
            for (const auto& source : sources) {
                if (dryRun) std::cout << "Would permanently delete: " << source.string() << '\n';
                else fs::remove(source);
            }
            std::cout << (dryRun ? "Previewed " : "Deleted ") << sources.size() << " file(s).\n";
            return;
        }
        error(line, "unknown files action: " + action);
    }

    auto callFunction = [this, line](const std::string& name, const std::vector<std::string>& suppliedArguments) {
        const auto found = functions_.find(name);
        if (found == functions_.end()) return false;
        if (suppliedArguments.size() != found->second.parameters.size()) error(line, "wrong number of function arguments");
        Scope local;
        for (std::size_t index = 0; index < found->second.parameters.size(); ++index) {
            local[found->second.parameters[index]] = evaluate(suppliedArguments[index]);
        }
        scopes_.push_back(std::move(local));
        immutable_.emplace_back();
        try {
            executeBlock(found->second.body);
        } catch (...) {
            immutable_.pop_back();
            scopes_.pop_back();
            throw;
        }
        immutable_.pop_back();
        scopes_.pop_back();
        return true;
    };
    if (callFunction(arguments[0], std::vector<std::string>(arguments.begin() + 1, arguments.end()))) {
        return;
    }
    const auto openParenthesis = expanded.find('(');
    if (openParenthesis != std::string::npos && expanded.back() == ')') {
        const auto name = trim(expanded.substr(0, openParenthesis));
        const auto supplied = splitArguments(expanded.substr(openParenthesis + 1, expanded.size() - openParenthesis - 2));
        if (callFunction(name, supplied)) return;
    }

    if (first == "run") {
        const auto toRun = trim(expanded.substr(arguments[0].size()));
        if (toRun.empty()) error(line, "run needs a Windows command");
        const auto result = std::system(toRun.c_str());
        std::cout << "External command exited with " << result << '\n';
        return;
    }

    error(line, "unknown command: " + arguments[0] + ". Type 'help'.");
}

Value Interpreter::evaluate(const std::string& expression) const {
    const auto text = trim(expand(expression));
    if (text.empty() || text == "null") return Value{};
    if (text == "true") return Value{true};
    if (text == "false") return Value{false};
    if (startsWith(text, "files count ")) {
        const auto arguments = splitArguments(text.substr(12));
        if (arguments.size() != 1) {
            throw std::runtime_error("files count expression needs one file pattern");
        }
        return Value{static_cast<long long>(expandFilePattern(unquote(arguments[0])).size())};
    }
    if (text.front() == '[' && text.back() == ']') {
        Value::List list;
        for (const auto& item : splitArguments(text.substr(1, text.size() - 2))) {
            if (!trim(item).empty()) list.push_back(unquote(trim(item)));
        }
        return Value{list};
    }
    if ((text.front() == '"' && text.back() == '"') || (text.front() == '\'' && text.back() == '\'')) {
        return Value{unquote(text)};
    }
    try {
        std::size_t consumed = 0;
        const auto number = std::stoll(text, &consumed);
        if (consumed == text.size()) return Value{number};
    } catch (const std::exception&) {
        // Not an integer; treat it as text below.
    }
    if (const auto found = variable(text)) return *found;
    return Value{text};
}

bool Interpreter::evaluateCondition(const std::string& condition) const {
    const auto text = trim(condition);
    for (const auto& operation : {std::string("=="), std::string("!="), std::string(">="), std::string("<="), std::string(">"), std::string("<")}) {
        const auto position = text.find(operation);
        if (position == std::string::npos) continue;
        const auto left = evaluate(text.substr(0, position));
        const auto right = evaluate(text.substr(position + operation.size()));
        const auto leftNumber = std::get_if<long long>(&left.data);
        const auto rightNumber = std::get_if<long long>(&right.data);
        if (leftNumber && rightNumber) {
            if (operation == "==") return *leftNumber == *rightNumber;
            if (operation == "!=") return *leftNumber != *rightNumber;
            if (operation == ">=") return *leftNumber >= *rightNumber;
            if (operation == "<=") return *leftNumber <= *rightNumber;
            if (operation == ">") return *leftNumber > *rightNumber;
            return *leftNumber < *rightNumber;
        }
        const auto lhs = left.toString();
        const auto rhs = right.toString();
        if (operation == "==") return lhs == rhs;
        if (operation == "!=") return lhs != rhs;
        error(0, "only numbers can use ordering comparisons");
    }
    return evaluate(text).truthy();
}

std::string Interpreter::expand(const std::string& text) const {
    std::string result;
    for (std::size_t index = 0; index < text.size();) {
        if (text[index] != '$') {
            result += text[index++];
            continue;
        }
        std::size_t start = ++index;
        std::string name;
        if (index < text.size() && text[index] == '{') {
            start = ++index;
            const auto end = text.find('}', index);
            if (end == std::string::npos) return text;
            name = text.substr(start, end - start);
            index = end + 1;
        } else {
            while (index < text.size() && (std::isalnum(static_cast<unsigned char>(text[index])) || text[index] == '_')) ++index;
            name = text.substr(start, index - start);
        }
        if (name.empty()) {
            result += '$';
            continue;
        }
        if (const auto found = variable(name)) result += found->toString();
        else result += '$' + name;
    }
    return result;
}

std::optional<Value> Interpreter::variable(const std::string& name) const {
    for (auto scope = scopes_.rbegin(); scope != scopes_.rend(); ++scope) {
        const auto found = scope->find(name);
        if (found != scope->end()) return found->second;
    }
    return std::nullopt;
}

void Interpreter::assign(const std::string& name, Value value, bool immutable) {
    if (!std::regex_match(name, std::regex("[A-Za-z_][A-Za-z0-9_]*"))) {
        throw std::runtime_error("invalid variable name: " + name);
    }
    if (immutable_.back().contains(name)) {
        throw std::runtime_error("cannot change constant: " + name);
    }
    scopes_.back()[name] = std::move(value);
    if (immutable) immutable_.back().insert(name);
}

std::string Interpreter::trim(std::string text) {
    const auto notSpace = [](unsigned char character) { return !std::isspace(character); };
    text.erase(text.begin(), std::find_if(text.begin(), text.end(), notSpace));
    text.erase(std::find_if(text.rbegin(), text.rend(), notSpace).base(), text.end());
    return text;
}

std::string Interpreter::lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
    return text;
}

std::vector<std::string> Interpreter::splitArguments(const std::string& text) {
    std::vector<std::string> result;
    std::string current;
    char quote = '\0';
    int brackets = 0;
    for (const char character : text) {
        if (quote != '\0') {
            current += character;
            if (character == quote) quote = '\0';
            continue;
        }
        if (character == '"' || character == '\'') {
            quote = character;
            current += character;
        } else if (character == '[') {
            ++brackets;
            current += character;
        } else if (character == ']') {
            --brackets;
            current += character;
        } else if ((std::isspace(static_cast<unsigned char>(character)) || character == ',') && brackets == 0) {
            if (!current.empty()) {
                result.push_back(current);
                current.clear();
            }
        } else {
            current += character;
        }
    }
    if (quote != '\0') throw std::runtime_error("unterminated quoted value");
    if (!current.empty()) result.push_back(current);
    return result;
}

std::string Interpreter::unquote(const std::string& text) {
    if (text.size() >= 2 && ((text.front() == '"' && text.back() == '"') || (text.front() == '\'' && text.back() == '\''))) {
        return text.substr(1, text.size() - 2);
    }
    return text;
}

bool Interpreter::wildcardMatch(const std::string& name, const std::string& pattern) {
    std::size_t nameIndex = 0, patternIndex = 0, star = std::string::npos, match = 0;
    while (nameIndex < name.size()) {
        if (patternIndex < pattern.size() && (pattern[patternIndex] == '?' || std::tolower(static_cast<unsigned char>(pattern[patternIndex])) == std::tolower(static_cast<unsigned char>(name[nameIndex])))) {
            ++nameIndex; ++patternIndex;
        } else if (patternIndex < pattern.size() && pattern[patternIndex] == '*') {
            star = patternIndex++;
            match = nameIndex;
        } else if (star != std::string::npos) {
            patternIndex = star + 1;
            nameIndex = ++match;
        } else return false;
    }
    while (patternIndex < pattern.size() && pattern[patternIndex] == '*') ++patternIndex;
    return patternIndex == pattern.size();
}

std::vector<fs::path> Interpreter::expandFilePattern(const std::string& pattern) {
    const fs::path fullPattern = pattern;
    const auto filenamePattern = fullPattern.filename().string();
    const auto folder = fullPattern.has_parent_path() ? fullPattern.parent_path() : fs::current_path();
    std::vector<fs::path> matches;
    if (!fs::exists(folder)) return matches;
    if (!hasWildcard(filenamePattern)) {
        if (fs::exists(fullPattern)) matches.push_back(fullPattern);
        return matches;
    }
    for (const auto& entry : fs::directory_iterator(folder)) {
        if (entry.is_regular_file() && wildcardMatch(entry.path().filename().string(), filenamePattern)) matches.push_back(entry.path());
    }
    return matches;
}

[[noreturn]] void Interpreter::error(std::size_t line, const std::string& message) const {
    const auto position = line == 0 ? "" : origin_ + ":" + std::to_string(line) + ": ";
    throw std::runtime_error(position + message);
}

} // namespace hinegx
