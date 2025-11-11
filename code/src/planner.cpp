#include <iostream>
#include <fstream>
// #include <boost/functional/hash.hpp>
#include <regex>
#include <unordered_set>
#include <set>
#include <list>
#include <unordered_map>
#include <algorithm>
#include <stdexcept>
#include <queue>
#include <cstring>
#include <climits>
#include <chrono>

#define SYMBOLS 0
#define INITIAL 1
#define GOAL 2
#define ACTIONS 3
#define ACTION_DEFINITION 4
#define ACTION_PRECONDITION 5
#define ACTION_EFFECT 6
#ifndef ENVS_DIR
#define ENVS_DIR "../envs"
#endif
class GroundedCondition;
class Condition;
class GroundedAction;
class Action;
class Env;

using namespace std;

bool print_status = true;

int max_effect_size = 0;
bool enable_heuristics = true;

string heuristic_fn = "edl";

class GroundedCondition
{
    string predicate;
    list<string> arg_values;
    bool truth = true;

public:
    GroundedCondition(const string &predicate, const list<string> &arg_values, bool truth = true)
    {
        this->predicate = predicate;
        this->truth = truth; // fixed
        for (const string &l : arg_values)
        {
            this->arg_values.push_back(l);
        }
    }

    GroundedCondition(const GroundedCondition &gc)
    {
        // Copy constructor
        this->predicate = gc.predicate;
        this->truth = gc.truth; // fixed
        for (const string &l : gc.arg_values)
        {
            this->arg_values.push_back(l);
        }
    }

    string get_predicate() const
    {
        return this->predicate;
    }
    list<string> get_arg_values() const
    {
        return this->arg_values;
    }

    bool get_truth() const
    {
        return this->truth;
    }

    friend ostream &operator<<(ostream &os, const GroundedCondition &pred)
    {
        os << pred.toString() << " ";
        return os;
    }

    bool operator==(const GroundedCondition &rhs) const
    {
        if (this->predicate != rhs.predicate || this->arg_values.size() != rhs.arg_values.size())
            return false;

        auto lhs_it = this->arg_values.begin();
        auto rhs_it = rhs.arg_values.begin();

        while (lhs_it != this->arg_values.end() && rhs_it != rhs.arg_values.end())
        {
            if (*lhs_it != *rhs_it)
                return false;
            ++lhs_it;
            ++rhs_it;
        }

        if (this->truth != rhs.get_truth()) // fixed
            return false;

        return true;
    }

    string toString() const
    {
        string temp;
        temp += this->predicate;
        temp += "(";
        for (const string &l : this->arg_values)
        {
            temp += l + ",";
        }
        temp = temp.substr(0, temp.length() - 1);
        temp += ")";
        return temp;
    }
};

struct GroundedConditionComparator
{
    bool operator()(const GroundedCondition &lhs, const GroundedCondition &rhs) const
    {
        return lhs == rhs;
    }
};

struct GroundedConditionHasher
{
    size_t operator()(const GroundedCondition &gcond) const
    {
        return hash<string>{}(gcond.toString());
    }
};

class Condition
{
    string predicate;
    list<string> args;
    bool truth;

public:
    Condition(const string &pred, const list<string> &args, const bool truth)
    {
        this->predicate = pred;
        this->truth = truth;
        for (const string &ar : args)
        {
            this->args.push_back(ar);
        }
    }

    string get_predicate() const
    {
        return this->predicate;
    }

    list<string> get_args() const
    {
        return this->args;
    }

    bool get_truth() const
    {
        return this->truth;
    }

    friend ostream &operator<<(ostream &os, const Condition &cond)
    {
        os << cond.toString() << " ";
        return os;
    }

    bool operator==(const Condition &rhs) const // fixed
    {

        if (this->predicate != rhs.predicate || this->args.size() != rhs.args.size())
            return false;

        auto lhs_it = this->args.begin();
        auto rhs_it = rhs.args.begin();

        while (lhs_it != this->args.end() && rhs_it != rhs.args.end())
        {
            if (*lhs_it != *rhs_it)
                return false;
            ++lhs_it;
            ++rhs_it;
        }

        if (this->truth != rhs.get_truth())
            return false;

        return true;
    }

    string toString() const
    {
        string temp;
        if (!this->truth)
            temp += "!";
        temp += this->predicate;
        temp += "(";
        for (const string &l : this->args)
        {
            temp += l + ",";
        }
        temp = temp.substr(0, temp.length() - 1);
        temp += ")";
        return temp;
    }
};

struct ConditionComparator
{
    bool operator()(const Condition &lhs, const Condition &rhs) const
    {
        return lhs == rhs;
    }
};

struct ConditionHasher
{
    size_t operator()(const Condition &cond) const
    {
        return hash<string>{}(cond.toString());
    }
};

class Action
{
    string name;
    list<string> args;
    unordered_set<Condition, ConditionHasher, ConditionComparator> preconditions;
    unordered_set<Condition, ConditionHasher, ConditionComparator> effects;

public:
    Action(const string &name, const list<string> &args,
           const unordered_set<Condition, ConditionHasher, ConditionComparator> &preconditions,
           const unordered_set<Condition, ConditionHasher, ConditionComparator> &effects)
    {
        this->name = name;
        for (const string &l : args)
        {
            this->args.push_back(l);
        }
        for (const Condition &pc : preconditions)
        {
            this->preconditions.insert(pc);
        }
        for (const Condition &pc : effects)
        {
            this->effects.insert(pc);
        }
    }
    string get_name() const
    {
        return this->name;
    }
    list<string> get_args() const
    {
        return this->args;
    }
    unordered_set<Condition, ConditionHasher, ConditionComparator> get_preconditions() const
    {
        return this->preconditions;
    }
    unordered_set<Condition, ConditionHasher, ConditionComparator> get_effects() const
    {
        return this->effects;
    }

    bool operator==(const Action &rhs) const
    {
        if (this->get_name() != rhs.get_name() || this->get_args().size() != rhs.get_args().size())
            return false;

        return true;
    }

    friend ostream &operator<<(ostream &os, const Action &ac)
    {
        os << ac.toString() << endl;
        os << "Precondition: ";
        for (const Condition &precond : ac.get_preconditions())
            os << precond;
        os << endl;
        os << "Effect: ";
        for (const Condition &effect : ac.get_effects())
            os << effect;
        os << endl;
        return os;
    }

    string toString() const
    {
        string temp;
        temp += this->get_name();
        temp += "(";
        for (const string &l : this->get_args())
        {
            temp += l + ",";
        }
        temp = temp.substr(0, temp.length() - 1);
        temp += ")";
        return temp;
    }
};

struct ActionComparator
{
    bool operator()(const Action &lhs, const Action &rhs) const
    {
        return lhs == rhs;
    }
};

struct ActionHasher
{
    size_t operator()(const Action &ac) const
    {
        return hash<string>{}(ac.get_name());
    }
};

class Env
{
    unordered_set<GroundedCondition, GroundedConditionHasher, GroundedConditionComparator> initial_conditions;
    unordered_set<GroundedCondition, GroundedConditionHasher, GroundedConditionComparator> goal_conditions;
    unordered_set<Action, ActionHasher, ActionComparator> actions;
    unordered_set<string> symbols;

public:
    void remove_initial_condition(const GroundedCondition &gc)
    {
        this->initial_conditions.erase(gc);
    }
    void add_initial_condition(const GroundedCondition &gc)
    {
        this->initial_conditions.insert(gc);
    }
    void add_goal_condition(const GroundedCondition &gc)
    {
        this->goal_conditions.insert(gc);
    }
    void remove_goal_condition(const GroundedCondition &gc)
    {
        this->goal_conditions.erase(gc);
    }
    void add_symbol(const string &symbol)
    {
        symbols.insert(symbol);
    }
    void add_symbols(const list<string> &symbols)
    {
        for (const string &l : symbols)
            this->symbols.insert(l);
    }
    void add_action(const Action &action)
    {
        this->actions.insert(action);
    }

    Action get_action(const string &name) const
    {
        for (Action a : this->actions)
        {
            if (a.get_name() == name)
                return a;
        }
        throw runtime_error("Action " + name + " not found!");
    }

    unordered_set<string> get_symbols() const
    {
        return this->symbols;
    }

    friend ostream &operator<<(ostream &os, const Env &w)
    {
        os << "***** Environment *****" << endl
           << endl;
        os << "Symbols: ";
        for (const string &s : w.get_symbols())
            os << s + ",";
        os << endl;
        os << "Initial conditions: ";
        for (const GroundedCondition &s : w.initial_conditions)
            os << s;
        os << endl;
        os << "Goal conditions: ";
        for (const GroundedCondition &g : w.goal_conditions)
            os << g;
        os << endl;
        os << "Actions:" << endl;
        for (const Action &g : w.actions)
            os << g << endl;
        cout << "***** Environment Created! *****" << endl;
        return os;
    }

    // add getters
    auto get_initial_conditions() const
    {
        return this->initial_conditions;
    }
    auto get_goal_conditions() const
    {
        return this->goal_conditions;
    }
    auto get_actions() const
    {
        return this->actions;
    }
};

class GroundedAction
{
    string name;
    list<string> arg_values;
    unordered_set<GroundedCondition, GroundedConditionHasher, GroundedConditionComparator> grounded_preconditions;
    unordered_set<GroundedCondition, GroundedConditionHasher, GroundedConditionComparator> grounded_effects;

public:
    GroundedAction(const string &name, const list<string> &arg_values)
    {
        this->name = name;
        for (const string &ar : arg_values)
        {
            this->arg_values.push_back(ar);
        }
    }

    // New constructor that accepts grounded preconditions and effects
    GroundedAction(const string &name, const list<string> &arg_values,
                   const unordered_set<GroundedCondition, GroundedConditionHasher, GroundedConditionComparator> &preconds,
                   const unordered_set<GroundedCondition, GroundedConditionHasher, GroundedConditionComparator> &effects)
    {
        this->name = name;
        for (const string &ar : arg_values)
        {
            this->arg_values.push_back(ar);
        }
        for (const auto &pc : preconds)
            this->grounded_preconditions.insert(pc);
        for (const auto &ef : effects)
            this->grounded_effects.insert(ef);
    }

    // Accessors for grounded preconditions/effects
    auto get_grounded_preconditions() const {
        return this->grounded_preconditions;
    }

    auto get_grounded_effects() const {
        return this->grounded_effects;
    }

    string get_name() const
    {
        return this->name;
    }

    list<string> get_arg_values() const
    {
        return this->arg_values;
    }

    bool operator==(const GroundedAction &rhs) const
    {
        if (this->name != rhs.name || this->arg_values.size() != rhs.arg_values.size())
            return false;

        auto lhs_it = this->arg_values.begin();
        auto rhs_it = rhs.arg_values.begin();

        while (lhs_it != this->arg_values.end() && rhs_it != rhs.arg_values.end())
        {
            if (*lhs_it != *rhs_it)
                return false;
            ++lhs_it;
            ++rhs_it;
        }
        return true;
    }

    friend ostream &operator<<(ostream &os, const GroundedAction &gac)
    {
        os << gac.toString() << " ";
        return os;
    }

    string toString() const
    {
        string temp;
        temp += this->name;
        temp += "(";
        for (const string &l : this->arg_values)
        {
            temp += l + ",";
        }
        temp = temp.substr(0, temp.length() - 1);
        temp += ")";
        return temp;
    }
};

struct State
{
   unordered_set<GroundedCondition, GroundedConditionHasher, GroundedConditionComparator> conditions;
   float g;
   float h;
   float f;
   State* parent;
   GroundedAction* parent_action;
};

struct CompareF
{
    bool operator()(const State *a, const State *b) const
    {
        return a->f > b->f; // lowest f at the top
    }
};

struct StateHasher
{
    size_t operator()(const State* state) const
    {
        size_t seed = 0;
        for (const auto& cond : state->conditions) {
            seed ^= GroundedConditionHasher{}(cond) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
        return seed;
    }
};

struct StateComparator
{
    bool operator()(const State* lhs, const State* rhs) const
    {
        if (lhs->conditions.size() != rhs->conditions.size())
            return false;

        for (const auto& cond : lhs->conditions) {
            if (rhs->conditions.find(cond) == rhs->conditions.end()) {
                return false;
            }
        }
        return true;
    }
};

std::vector<string> getStateConditionsAsStrings(const State* state) {
    std::vector<string> conditionStrings;
    for (const auto& cond : state->conditions) {
        conditionStrings.push_back(cond.toString());
    }
    return conditionStrings;
}

struct VectorStringHasher
{
    size_t operator()(const vector<string> &vec) const
    {
        size_t seed = 0;
        for (const auto &s : vec)
        {
            seed ^= std::hash<string>{}(s);  // XOR is commutative
        }
        return seed;
    }
};

struct VectorStringComparator
{
    bool operator()(const vector<string> &a, const vector<string> &b) const
    {
        if (a.size() != b.size()) return false;

        vector<string> a_sorted = a;
        vector<string> b_sorted = b;
        sort(a_sorted.begin(), a_sorted.end());
        sort(b_sorted.begin(), b_sorted.end());

        for (size_t i = 0; i < a_sorted.size(); ++i)
        {
            if (a_sorted[i] != b_sorted[i])
                return false;
        }
        return true;
    }
};

list<string> parse_symbols(string symbols_str)
{
    list<string> symbols;
    size_t pos = 0;
    string delimiter = ",";
    while ((pos = symbols_str.find(delimiter)) != string::npos)
    {
        string symbol = symbols_str.substr(0, pos);
        symbols_str.erase(0, pos + delimiter.length());
        symbols.push_back(symbol);
    }
    symbols.push_back(symbols_str);
    return symbols;
}

Env *create_env(char *filename)
{
    ifstream input_file(filename);
    Env *env = new Env();
    regex symbolStateRegex("symbols:", regex::icase);
    regex symbolRegex("([a-zA-Z0-9_, ]+) *");
    regex initialConditionRegex("initialconditions:(.*)", regex::icase);
    regex conditionRegex("(!?[A-Z][a-zA-Z_]*) *\\( *([a-zA-Z0-9_, ]+) *\\)");
    regex goalConditionRegex("goalconditions:(.*)", regex::icase);
    regex actionRegex("actions:", regex::icase);
    regex precondRegex("preconditions:(.*)", regex::icase);
    regex effectRegex("effects:(.*)", regex::icase);
    int parser = SYMBOLS;

    unordered_set<Condition, ConditionHasher, ConditionComparator> preconditions;
    unordered_set<Condition, ConditionHasher, ConditionComparator> effects;
    string action_name;
    string action_args;

    string line;
    if (input_file.is_open())
    {
        while (getline(input_file, line))
        {
            string::iterator end_pos = remove(line.begin(), line.end(), ' ');
            line.erase(end_pos, line.end());

            if (line.empty())
                continue;

            if (parser == SYMBOLS)
            {
                smatch results;
                if (regex_search(line, results, symbolStateRegex))
                {
                    line = line.substr(8);
                    sregex_token_iterator iter(line.begin(), line.end(), symbolRegex, 0);
                    sregex_token_iterator end;

                    env->add_symbols(parse_symbols(iter->str())); // fixed

                    parser = INITIAL;
                }
                else
                {
                    cout << "Symbols are not specified correctly." << endl;
                    throw;
                }
            }
            else if (parser == INITIAL)
            {
                const char *line_c = line.c_str();
                if (regex_match(line_c, initialConditionRegex))
                {
                    const std::vector<int> submatches = {1, 2};
                    sregex_token_iterator iter(
                        line.begin(), line.end(), conditionRegex, submatches);
                    sregex_token_iterator end;

                    while (iter != end)
                    {
                        // name
                        string predicate = iter->str();
                        iter++;
                        // args
                        string args = iter->str();
                        iter++;

                        if (predicate[0] == '!')
                        {
                            env->remove_initial_condition(
                                GroundedCondition(predicate.substr(1), parse_symbols(args)));
                        }
                        else
                        {
                            env->add_initial_condition(
                                GroundedCondition(predicate, parse_symbols(args)));
                        }
                    }

                    parser = GOAL;
                }
                else
                {
                    cout << "Initial conditions not specified correctly." << endl;
                    throw;
                }
            }
            else if (parser == GOAL)
            {
                const char *line_c = line.c_str();
                if (regex_match(line_c, goalConditionRegex))
                {
                    const std::vector<int> submatches = {1, 2};
                    sregex_token_iterator iter(
                        line.begin(), line.end(), conditionRegex, submatches);
                    sregex_token_iterator end;

                    while (iter != end)
                    {
                        // name
                        string predicate = iter->str();
                        iter++;
                        // args
                        string args = iter->str();
                        iter++;

                        if (predicate[0] == '!')
                        {
                            env->remove_goal_condition(
                                GroundedCondition(predicate.substr(1), parse_symbols(args)));
                        }
                        else
                        {
                            env->add_goal_condition(
                                GroundedCondition(predicate, parse_symbols(args)));
                        }
                    }

                    parser = ACTIONS;
                }
                else
                {
                    cout << "Goal conditions not specified correctly." << endl;
                    throw;
                }
            }
            else if (parser == ACTIONS)
            {
                const char *line_c = line.c_str();
                if (regex_match(line_c, actionRegex))
                {
                    parser = ACTION_DEFINITION;
                }
                else
                {
                    cout << "Actions not specified correctly." << endl;
                    throw;
                }
            }
            else if (parser == ACTION_DEFINITION)
            {
                const char *line_c = line.c_str();
                if (regex_match(line_c, conditionRegex))
                {
                    const std::vector<int> submatches = {1, 2};
                    sregex_token_iterator iter(
                        line.begin(), line.end(), conditionRegex, submatches);
                    sregex_token_iterator end;
                    // name
                    action_name = iter->str();
                    iter++;
                    // args
                    action_args = iter->str();
                    iter++;

                    parser = ACTION_PRECONDITION;
                }
                else
                {
                    cout << "Action not specified correctly." << endl;
                    throw;
                }
            }
            else if (parser == ACTION_PRECONDITION)
            {
                const char *line_c = line.c_str();
                if (regex_match(line_c, precondRegex))
                {
                    const std::vector<int> submatches = {1, 2};
                    sregex_token_iterator iter(
                        line.begin(), line.end(), conditionRegex, submatches);
                    sregex_token_iterator end;

                    while (iter != end)
                    {
                        // name
                        string predicate = iter->str();
                        iter++;
                        // args
                        string args = iter->str();
                        iter++;

                        bool truth;

                        if (predicate[0] == '!')
                        {
                            predicate = predicate.substr(1);
                            truth = false;
                        }
                        else
                        {
                            truth = true;
                        }

                        Condition precond(predicate, parse_symbols(args), truth);
                        preconditions.insert(precond);
                    }

                    parser = ACTION_EFFECT;
                }
                else
                {
                    cout << "Precondition not specified correctly." << endl;
                    throw;
                }
            }
            else if (parser == ACTION_EFFECT)
            {
                const char *line_c = line.c_str();
                if (regex_match(line_c, effectRegex))
                {
                    const std::vector<int> submatches = {1, 2};
                    sregex_token_iterator iter(
                        line.begin(), line.end(), conditionRegex, submatches);
                    sregex_token_iterator end;

                    while (iter != end)
                    {
                        // name
                        string predicate = iter->str();
                        iter++;
                        // args
                        string args = iter->str();
                        iter++;

                        bool truth;

                        if (predicate[0] == '!')
                        {
                            predicate = predicate.substr(1);
                            truth = false;
                        }
                        else
                        {
                            truth = true;
                        }

                        Condition effect(predicate, parse_symbols(args), truth);
                        effects.insert(effect);
                    }

                    env->add_action(
                        Action(action_name, parse_symbols(action_args), preconditions, effects));

                    preconditions.clear();
                    effects.clear();
                    parser = ACTION_DEFINITION;
                }
                else
                {
                    cout << "Effects not specified correctly." << endl;
                    throw;
                }
            }
        }
        input_file.close();
    }

    else
        cout << "Unable to open file";

    return env;
}

void generateGroundedCombinations(
    Action action,
    vector<string> currArgs,
    vector<GroundedAction> &groundedActions,
    unordered_set<string> symbols,
    int nArgs
)
{
    if (currArgs.size() == nArgs)
    {
        // Build mapping from action parameter names to grounded argument values
        list<string> paramList = action.get_args();
        vector<string> params(paramList.begin(), paramList.end());
        vector<string> groundedArgs = currArgs; // same order as params

        // Grounded preconditions/effects
        unordered_set<GroundedCondition, GroundedConditionHasher, GroundedConditionComparator> gPreconds;
        unordered_set<GroundedCondition, GroundedConditionHasher, GroundedConditionComparator> gEffects;

        // Ground each precondition by substituting parameters with groundedArgs
        for (const Condition &cond : action.get_preconditions()) {
            list<string> condArgs = cond.get_args();
            list<string> groundedCondArgs;
            for (const string &a : condArgs) {
                // find if 'a' is one of the parameters; if so, substitute
                auto it = find(params.begin(), params.end(), a);
                if (it != params.end()) {
                    int idx = distance(params.begin(), it);
                    groundedCondArgs.push_back(groundedArgs[idx]);
                } else {
                    groundedCondArgs.push_back(a);
                }
            }
            gPreconds.insert(GroundedCondition(cond.get_predicate(), groundedCondArgs, cond.get_truth()));
        }

        // Ground each effect similarly
        int effectSize = 0;
        for (const Condition &cond : action.get_effects()) {
            list<string> condArgs = cond.get_args();
            list<string> groundedCondArgs;
            for (const string &a : condArgs) {
                auto it = find(params.begin(), params.end(), a);
                if (it != params.end()) {
                    int idx = distance(params.begin(), it);
                    groundedCondArgs.push_back(groundedArgs[idx]);
                } else {
                    groundedCondArgs.push_back(a);
                }
            }
            gEffects.insert(GroundedCondition(cond.get_predicate(), groundedCondArgs, cond.get_truth()));
            effectSize++;
        }

        // Keep track of largest effect size to scale the hamming distance for h value
        if(effectSize > max_effect_size){
            max_effect_size = effectSize;
        }

        groundedActions.push_back(GroundedAction(action.get_name(), list<string>(currArgs.begin(), currArgs.end()), gPreconds, gEffects));
        return;
    }

    for (const string &symbol : symbols)
    {
        currArgs.push_back(symbol);
        unordered_set<string> remainingSymbols = symbols;
        remainingSymbols.erase(symbol);
        generateGroundedCombinations(action, currArgs, groundedActions, remainingSymbols, nArgs);
        currArgs.pop_back();
    }
}

std::vector<GroundedAction> generateAllGroundedActions(Env* env) {
    std::vector<GroundedAction> groundedActions;
    unordered_set<Action, ActionHasher, ActionComparator> actions = env->get_actions();
    unordered_set<string> symbols = env->get_symbols();

    for (const Action &action : actions) {
        vector<string> currArgs;
        int nArgs = action.get_args().size();
        generateGroundedCombinations(action, currArgs, groundedActions, symbols, nArgs);
    }

    return groundedActions;
}

std::vector<GroundedAction> getApplicableActions(State* state, Env* env, std::vector<GroundedAction>& allActions) {
    std::vector<GroundedAction> validActions;

    for (const auto &gaction : allActions) {
        bool applicable = true;
        auto preconds = gaction.get_grounded_preconditions();
        for (const auto &pc : preconds) {
            if (pc.get_truth()) {
                // positive precondition: must be present
                if (state->conditions.find(pc) == state->conditions.end()) {
                    applicable = false;
                    break;
                }
            } else {
                // negative precondition: the positive condition must NOT be present
                GroundedCondition positive(pc.get_predicate(), pc.get_arg_values(), true);
                if (state->conditions.find(positive) != state->conditions.end()) {
                    applicable = false;
                    break;
                }
            }
        }

        if (applicable) {
            validActions.push_back(gaction);
        }
    }

    return validActions;
}


float getHeuristicHam(State* state, State* goal){
    if (max_effect_size <= 0) {
        throw runtime_error("max_effect_size is less than or equal to 0");
    }

    // Heuristic: number of goal conditions that are not satisfied in the given state.
    float missing = 0;
    for (const auto &gcond : goal->conditions) {
        if (gcond.get_truth()) {
            // positive goal condition: must be present in state
            if (state->conditions.find(gcond) == state->conditions.end()) {
                missing++;
            }
        } else {
            // negative goal condition: the positive version must NOT be present
            GroundedCondition positive(gcond.get_predicate(), gcond.get_arg_values(), true);
            if (state->conditions.find(positive) != state->conditions.end()) {
                missing++;
            }
        }
    }

    // Make the h value admissable
    missing = missing / max_effect_size;
    return missing;
}

float getHeuristicEDL(State* state, State* goal){
    return 0.0;
}


float getHeristic(State* state, State* goal){

    float h_val = 0.0;

    if (!enable_heuristics) {
        return 0.0;
    }

    if (heuristic_fn == "ham"){
        h_val = getHeuristicHam(state, goal);
        return h_val;
    }

    if(heuristic_fn == "edl"){
        h_val = getHeuristicEDL(state, goal);
        return h_val;
    }

    return 0.0;
}

list<GroundedAction> planner(Env *env)
{
    //////////////////////////////////////////
    ///// TODO: INSERT YOUR PLANNER HERE /////
    //////////////////////////////////////////

    // Blocks World example (TODO: CHANGE THIS)
    // cout << endl
    //      << "CREATING DEFAULT PLAN" << endl;
    // list<GroundedAction> actions;
    // actions.push_back(GroundedAction("MoveToTable", {"A", "B"}));
    // actions.push_back(GroundedAction("Move", {"C", "Table", "A"}));
    // actions.push_back(GroundedAction("Move", {"B", "Table", "C"}));

    ////// My Planner Implementation (A* Search) /////////

    // Start timing
    auto start_time = std::chrono::high_resolution_clock::now();

    list<GroundedAction> actions;

    // Counter for states expanded
    int states_expanded = 0;

    vector<GroundedAction> allActions = generateAllGroundedActions(env);

    cout << "Enable Heuristics: " << enable_heuristics << endl;
    cout << "Max Effect Size: " << max_effect_size << endl;
    cout << "Heuristic Function: " << heuristic_fn << endl;

    // Print all the grounded actions with their grounded preconditions and effects
    if (print_status && false) {
        cout << "All Grounded Actions:" << endl;
        for (const auto& action : allActions) {
            cout << action.toString() << endl;

            cout << "  Grounded Preconditions: ";
            auto gpre = action.get_grounded_preconditions();
            if (gpre.empty()) {
                cout << "(none)";
            } else {
                for (const auto& pc : gpre) {
                    if (!pc.get_truth()) cout << "!";
                    cout << pc;
                }
            }
            cout << endl;

            cout << "  Grounded Effects: ";
            auto geff = action.get_grounded_effects();
            if (geff.empty()) {
                cout << "(none)";
            } else {
                for (const auto& ef : geff) {
                    if (!ef.get_truth()) cout << "!";
                    cout << ef;
                }
            }
            cout << endl << endl;
        }
    }

    // Start and Goal states
    unordered_set<GroundedCondition, GroundedConditionHasher, GroundedConditionComparator> initial_conditions = env->get_initial_conditions();
    unordered_set<GroundedCondition, GroundedConditionHasher, GroundedConditionComparator> goal_conditions = env->get_goal_conditions();

    // Open list (Priority Queue)
    priority_queue<State*, vector<State*>, CompareF> openList;

    // Closed list (Set)
    unordered_set<std::vector<string>, VectorStringHasher, VectorStringComparator> closedSet;

    // Best G values
    unordered_map<std::vector<string>, float, VectorStringHasher, VectorStringComparator> gValues;

    // Generate string from goal conditions for easy comparison
    State* goalState = new State;
    goalState->conditions = goal_conditions;
    goalState->g = INT_MAX;
    goalState->h = 0;
    goalState->f = INT_MAX;
    goalState->parent = nullptr;
    std::vector<string> GoalConditionsString = getStateConditionsAsStrings(goalState);

    sort(GoalConditionsString.begin(), GoalConditionsString.end());
    // Initialize the open list with the start state
    State* startState = new State;
    startState->conditions = initial_conditions;
    startState->g = 0;
    startState->h = getHeristic(startState, goalState); // TODO: Define heuristic function
    startState->f = startState->g + startState->h;
    startState->parent = nullptr;
    openList.push(startState);
    gValues[getStateConditionsAsStrings(startState)] = startState->g;

    while (!openList.empty()){

        // Print size of open list
        std::cout << "Open list size: " << openList.size() << "\r";

        // Get the state with the lowest f value
        State* currentState = openList.top();
        openList.pop();

        std::vector<string> stateConditionsString = getStateConditionsAsStrings(currentState);

        // Skip if already in closed set
        if (closedSet.count(stateConditionsString) > 0) {
            continue;
        }

        // Lazy deletion: Skip if this state has a higher g value than the best known g value
        if (gValues.find(stateConditionsString) != gValues.end() && currentState->g > gValues[stateConditionsString]) {
            continue;
        }

        // Increment states expanded counter
        states_expanded++;

        // Check if we reached the goal: all goal conditions must be present in the current state
        bool goalReached = true;
        for (const auto &gcond : goal_conditions) {
            if (currentState->conditions.find(gcond) == currentState->conditions.end()) {
                goalReached = false;
                break;
            }
        }
        if (goalReached) {
            State* curr = currentState;
            while(curr->parent != nullptr) {
                actions.push_front(*(curr->parent_action));
                curr = curr->parent;
            }
            break;
        }

        // Add neighbors to open list
        vector<GroundedAction> applicableActions = getApplicableActions(currentState, env, allActions);

        if (print_status and false) {
            cout << "\nExpanding state (" << stateConditionsString.size() << " conditions). Applicable actions: " << applicableActions.size() << endl;
            for (const auto &aa : applicableActions) {
                cout << "  - " << aa.toString() << endl;
            }
        }

        for (const GroundedAction &action : applicableActions) {
            // Generate new state by applying the action's grounded effects
            State* neighborState = new State;
            neighborState->conditions = currentState->conditions; // start from current

            // Apply effects: add positive effects, remove positive form of negative effects
            auto effects = action.get_grounded_effects();
            for (const auto &ef : effects) {
                if (ef.get_truth()) {
                    neighborState->conditions.insert(ef);
                } else {
                    // remove the positive version
                    GroundedCondition positive(ef.get_predicate(), ef.get_arg_values(), true);
                    neighborState->conditions.erase(positive);
                }
            }

            // Set costs and parent pointers
            float new_g = currentState->g + 1;
            neighborState->g = new_g;
            neighborState->parent = currentState;
            neighborState->h = getHeristic(neighborState, goalState);
            neighborState->f = neighborState->g + neighborState->h;

            // store a copy of the applied action on the heap so path reconstruction can reference it
            GroundedAction* parentActionCopy = new GroundedAction(action.get_name(), action.get_arg_values(), action.get_grounded_preconditions(), action.get_grounded_effects());
            neighborState->parent_action = parentActionCopy;

            // Build vector key for this state's conditions (order-insensitive hashing/comparison handles ordering)
            std::vector<string> neighborConditionsVec = getStateConditionsAsStrings(neighborState);

            // Skip if already closed
            if (closedSet.count(neighborConditionsVec) > 0) {
                delete parentActionCopy;
                delete neighborState;
                continue;
            }

            // If this path to neighbor is better than any previous, or unseen, push to open list
            if (gValues.find(neighborConditionsVec) == gValues.end() || new_g < gValues[neighborConditionsVec]) {
                gValues[neighborConditionsVec] = new_g;
                openList.push(neighborState);
            } else {
                // not better, discard
                delete parentActionCopy;
                delete neighborState;
            }
        }

        closedSet.insert(stateConditionsString);
        gValues[stateConditionsString] = currentState->g;
    }

    // End timing
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    // Print timing and statistics
    cout << "\n\nPlanning Statistics:" << endl;
    cout << "Time taken: " << duration.count() << " ms" << endl;
    cout << "States expanded: " << states_expanded << endl;

    return actions;
}

int main(int argc, char *argv[])
{
    // DO NOT CHANGE THIS FUNCTION
    // char *env_file = static_cast<char *>("example.txt");
    const char *env_file = "example.txt";
    if (argc > 1)
        env_file = argv[1];

    // Parse optional heuristic flag argument
    if (argc > 2) {
        string heuristic_arg = argv[2];
        if (heuristic_arg == "0" || heuristic_arg == "false") {
            enable_heuristics = false;
        } else if (heuristic_arg == "1" || heuristic_arg == "true") {
            enable_heuristics = true;
        }
    }

    // Parse optional heuristic function argument
    if (argc > 3) {
        string heuristic_fn_arg = argv[3];
        if (heuristic_fn_arg == "edl" || heuristic_fn_arg == "ham") {
            heuristic_fn = heuristic_fn_arg;
        }
    }

    std::string envsDirPath = ENVS_DIR;
    char *filename = new char[envsDirPath.length() + strlen(env_file) + 2];
    strcpy(filename, envsDirPath.c_str());
    strcat(filename, "/");
    strcat(filename, env_file);

    cout << "Environment: " << filename << endl;
    Env *env = create_env(filename);
    if (print_status)
    {
        cout << *env;
    }

    list<GroundedAction> actions = planner(env);

    cout << "\nPlan: " << endl;
    for (const GroundedAction &gac : actions)
    {
        cout << gac << endl;
    }

    delete env;
    return 0;
}