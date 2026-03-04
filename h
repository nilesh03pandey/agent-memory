[33mcommit 2f7484b8b200a44a8c70699b63db9e5b8e7dc8e6[m[33m ([m[1;36mHEAD[m[33m -> [m[1;32mmaster[m[33m)[m
Author: nilesh13github <pandeynilesh325a@gmail.com>
Date:   Sat Feb 28 11:11:25 2026 +0530

    removed the optional cuda dependency

[1mdiff --git a/__init__.py b/__init__.py[m
[1mnew file mode 100644[m
[1mindex 0000000..e69de29[m
[1mdiff --git a/__pycache__/__init__.cpython-312.pyc b/__pycache__/__init__.cpython-312.pyc[m
[1mnew file mode 100644[m
[1mindex 0000000..88d5abb[m
Binary files /dev/null and b/__pycache__/__init__.cpython-312.pyc differ
[1mdiff --git a/__pycache__/agent.cpython-312.pyc b/__pycache__/agent.cpython-312.pyc[m
[1mnew file mode 100644[m
[1mindex 0000000..d3b8a5d[m
Binary files /dev/null and b/__pycache__/agent.cpython-312.pyc differ
[1mdiff --git a/__pycache__/memory_engine.cpython-312.pyc b/__pycache__/memory_engine.cpython-312.pyc[m
[1mnew file mode 100644[m
[1mindex 0000000..5a8b368[m
Binary files /dev/null and b/__pycache__/memory_engine.cpython-312.pyc differ
[1mdiff --git a/__pycache__/metrics.cpython-312.pyc b/__pycache__/metrics.cpython-312.pyc[m
[1mnew file mode 100644[m
[1mindex 0000000..af96fef[m
Binary files /dev/null and b/__pycache__/metrics.cpython-312.pyc differ
[1mdiff --git a/__pycache__/server.cpython-312.pyc b/__pycache__/server.cpython-312.pyc[m
[1mnew file mode 100644[m
[1mindex 0000000..1532a68[m
Binary files /dev/null and b/__pycache__/server.cpython-312.pyc differ
[1mdiff --git a/agent.py b/agent.py[m
[1mnew file mode 100644[m
[1mindex 0000000..aa94801[m
[1m--- /dev/null[m
[1m+++ b/agent.py[m
[36m@@ -0,0 +1,403 @@[m
[32m+[m[32m"""[m
[32m+[m[32mcore/agent.py[m
[32m+[m
[32m+[m[32mThe Agent — autonomous, memory-driven, self-directing.[m
[32m+[m
[32m+[m[32mThis is not a wrapper around an LLM. This is an entity that:[m
[32m+[m[32m  1. Reads its identity from SOUL/IDENTITY/AGENTS/USER markdown files[m
[32m+[m[32m  2. Uses MemoryEngine to recall context before every response[m
[32m+[m[32m  3. Follows the OBSERVE → THINK → PLAN → ACT → REFLECT → STORE loop[m
[32m+[m[32m  4. Updates USER.md as it learns preferences[m
[32m+[m[32m  5. Consolidates episodic → semantic memory periodically[m
[32m+[m
[32m+[m[32mDesigned for experiments, prototyping, and elastic memory research.[m
[32m+[m[32mNot hardcoded. Not a SaaS product. A cognitive agent you can run locally.[m
[32m+[m[32m"""[m
[32m+[m
[32m+[m[32mimport os[m
[32m+[m[32mimport json[m
[32m+[m[32mimport time[m
[32m+[m[32mfrom datetime import datetime[m
[32m+[m[32mfrom typing import Optional, Callable[m
[32m+[m[32mfrom pathlib import Path[m
[32m+[m[32mfrom dataclasses import dataclass, field[m
[32m+[m
[32m+[m[32mfrom core.memory_engine import ([m
[32m+[m[32m    MemoryEngine, MemoryType, Importance, Memory, LocalStorage, ImportanceClassifier[m
[32m+[m[32m)[m
[32m+[m
[32m+[m
[32m+[m[32m# ─────────────────────────────────────────[m
[32m+[m[32m# THOUGHT TRACE (Transparent Reasoning)[m
[32m+[m[32m# ─────────────────────────────────────────[m
[32m+[m
[32m+[m[32m@dataclass[m
[32m+[m[32mclass ThoughtTrace:[m
[32m+[m[32m    """[m
[32m+[m[32m    The agent's internal reasoning log for one interaction.[m
[32m+[m[32m    Transparent by default — you can see what the agent was thinking.[m
[32m+[m[32m    """[m
[32m+[m[32m    timestamp:        str   = field(default_factory=lambda: datetime.now().isoformat())[m
[32m+[m[32m    observation:      str   = ""[m
[32m+[m[32m    memory_loaded:    dict  = field(default_factory=dict)[m
[32m+[m[32m    plan:             str   = ""[m
[32m+[m[32m    action_taken:     str   = ""[m
[32m+[m[32m    importance_score: str   = ""[m
[32m+[m[32m    memories_stored:  list  = field(default_factory=list)[m
[32m+[m[32m    consolidation:    list  = field(default_factory=list)[m
[32m+[m[32m    duration_ms:      float = 0[m
[32m+[m
[32m+[m[32m    def render(self) -> str:[m
[32m+[m[32m        ts = datetime.fromisoformat(self.timestamp).strftime("%H:%M:%S")[m
[32m+[m[32m        lines = [[m
[32m+[m[32m            f"[{ts}] ── AGENT THOUGHT TRACE ──────────────────────",[m
[32m+[m[32m            f"  OBSERVE  : {self.observation}",[m
[32m+[m[32m        ][m
[32m+[m[32m        if self.memory_loaded:[m
[32m+[m[32m            counts = {k: len(v) for k, v in self.memory_loaded.items() if v}[m
[32m+[m[32m            lines.append(f"  MEMORY   : {counts}")[m
[32m+[m[32m        lines.append(f"  PLAN     : {self.plan}")[m
[32m+[m[32m        lines.append(f"  ACT      : {self.action_taken}")[m
[32m+[m[32m        lines.append(f"  REFLECT  : importance={self.importance_score}")[m
[32m+[m[32m        if self.memories_stored:[m
[32m+[m[32m            lines.append(f"  STORED   : {len(self.memories_stored)} memory entries")[m
[32m+[m[32m        if self.consolidation:[m
[32m+[m[32m            lines.append(f"  DREAM    : {len(self.consolidation)} episodic→semantic")[m
[32m+[m[32m        lines.append(f"  DURATION : {self.duration_ms:.0f}ms")[m
[32m+[m[32m        lines.append("────────────────────────────────────────────────")[m
[32m+[m[32m        return "\n".join(lines)[m
[32m+[m
[32m+[m
[32m+[m[32m# ─────────────────────────────────────────[m
[32m+[m[32m# AGENT RESPONSE[m
[32m+[m[32m# ─────────────────────────────────────────[m
[32m+[m
[32m+[m[32m@dataclass[m
[32m+[m[32mclass AgentResponse:[m
[32m+[m[32m    """Complete output from a single agent interaction."""[m
[32m+[m[32m    agent_id:    str[m
[32m+[m[32m    session_id:  str[m
[32m+[m[32m    text:        str[m
[32m+[m[32m    thought:     ThoughtTrace[m
[32m+[m[32m    memory_stats: dict[m
[32m+[m
[32m+[m[32m    def to_dict(self) -> dict:[m
[32m+[m[32m        return {[m
[32m+[m[32m            "agent_id":    self.agent_id,[m
[32m+[m[32m            "session_id":  self.session_id,[m
[32m+[m[32m            "text":        self.text,[m
[32m+[m[32m            "thought":     self.thought.__dict__,[m
[32m+[m[32m            "memory_stats": self.memory_stats,[m
[32m+[m[32m        }[m
[32m+[m
[32m+[m
[32m+[m[32m# ─────────────────────────────────────────[m
[32m+[m[32m# LLM ADAPTER (pluggable)[m
[32m+[m[32m# ─────────────────────────────────────────[m
[32m+[m
[32m+[m[32mclass LLMAdapter:[m
[32m+[m[32m    """[m
[32m+[m[32m    Abstract LLM interface. Swap backends without changing the agent.[m
[32m+[m[32m    Implement `generate(prompt) -> str` for your provider.[m
[32m+[m[32m    """[m
[32m+[m[32m    def generate(self, prompt: str) -> str:[m
[32m+[m[32m        raise NotImplementedError[m
[32m+[m
[32m+[m[32m    def get_name(self) -> str:[m
[32m+[m[32m        return "base"[m
[32m+[m
[32m+[m
[32m+[m[32mclass VertexAIAdapter(LLMAdapter):[m
[32m+[m[32m    """Google Vertex AI / Gemini backend."""[m
[32m+[m[32m    def __init__(self, model: str = "gemini-2.0-flash-exp", project: str = None, location: str = "us-central1"):[m
[32m+[m[32m        import vertexai[m
[32m+[m[32m        from vertexai.generative_models import GenerativeModel[m
[32m+[m[32m        project = project or os.environ.get("GOOGLE_CLOUD_PROJECT", "dynamicstorage")[m
[32m+[m[32m        vertexai.init(project=project, location=location)[m
[32m+[m[32m        self.model = GenerativeModel(model)[m
[32m+[m[32m        self._name = model[m
[32m+[m
[32m+[m[32m    def generate(self, prompt: str) -> str:[m
[32m+[m[32m        response = self.model.generate_content(prompt)[m
[32m+[m[32m        return response.text[m
[32m+[m
[32m+[m[32m    def get_name(self) -> str:[m
[32m+[m[32m        return self._name[m
[32m+[m
[32m+[m
[32m+[m[32mclass AnthropicAdapter(LLMAdapter):[m
[32m+[m[32m    """Anthropic Claude backend."""[m
[32m+[m[32m    def __init__(self, model: str = "claude-sonnet-4-20250514"):[m
[32m+[m[32m        import anthropic[m
[32m+[m[32m        self.client = anthropic.Anthropic(api_key=os.environ.get("ANTHROPIC_API_KEY"))[m
[32m+[m[32m        self._model = model[m
[32m+[m
[32m+[m[32m    def generate(self, prompt: str) -> str:[m
[32m+[m[32m        msg = self.client.messages.create([m
[32m+[m[32m            model=self._model,[m
[32m+[m[32m            max_tokens=2048,[m
[32m+[m[32m            messages=[{"role": "user", "content": prompt}],[m
[32m+[m[32m        )[m
[32m+[m[32m        return msg.content[0].text[m
[32m+[m
[32m+[m[32m    def get_name(self) -> str:[m
[32m+[m[32m        return self._model[m
[32m+[m
[32m+[m
[32m+[m[32mclass MockAdapter(LLMAdapter):[m
[32m+[m[32m    """[m
[32m+[m[32m    No-LLM adapter for testing and development.[m
[32m+[m[32m    Returns a structured mock response that echoes back context.[m
[32m+[m[32m    """[m
[32m+[m[32m    def generate(self, prompt: str) -> str:[m
[32m+[m[32m        # Extract user input from prompt for the mock[m
[32m+[m[32m        if "USER INPUT:" in prompt:[m
[32m+[m[32m            user_part = prompt.split("USER INPUT:")[-1].split("INSTRUCTIONS:")[0].strip()[m
[32m+[m[32m        else:[m
[32m+[m[32m            user_part = prompt[-200:][m
[32m+[m[32m        return ([m
[32m+[m[32m            f"[MOCK AGENT RESPONSE]\n"[m
[32m+[m[32m            f"I received: \"{user_part[:100]}...\"\n"[m
[32m+[m[32m            f"Memory context was loaded. I would now respond based on it.\n"[m
[32m+[m[32m            f"(Replace MockAdapter with VertexAIAdapter or AnthropicAdapter for real responses.)"[m
[32m+[m[32m        )[m
[32m+[m
[32m+[m[32m    def get_name(self) -> str:[m
[32m+[m[32m        return "mock"[m
[32m+[m
[32m+[m
[32m+[m[32m# ─────────────────────────────────────────[m
[32m+[m[32m# IDENTITY LOADER[m
[32m+[m[32m# ─────────────────────────────────────────[m
[32m+[m
[32m+[m[32mdef load_identity(identity_dir: str = "./identity") -> dict:[m
[32m+[m[32m    """[m
[32m+[m[32m    Load the agent's identity documents (SOUL, IDENTITY, AGENTS, USER).[m
[32m+[m[32m    Returns a dict with their contents for prompt construction.[m
[32m+[m[32m    """[m
[32m+[m[32m    docs = {}[m
[32m+[m[32m    path = Path(identity_dir)[m
[32m+[m[32m    for fname in ["SOUL.md", "IDENTITY.md", "AGENTS.md", "USER.md"]:[m
[32m+[m[32m        fpath = path / fname[m
[32m+[m[32m        if fpath.exists():[m
[32m+[m[32m            docs[fname.replace(".md", "").lower()] = fpath.read_text()[m
[32m+[m[32m        else:[m
[32m+[m[32m            docs[fname.replace(".md", "").lower()] = ""[m
[32m+[m[32m    return docs[m
[32m+[m
[32m+[m
[32m+[m[32m# ─────────────────────────────────────────[m
[32m+[m[32m# THE AGENT[m
[32m+[m[32m# ─────────────────────────────────────────[m
[32m+[m
[32m+[m[32mclass Agent:[m
[32m+[m[32m    """[m
[32m+[m[32m    An autonomous, memory-driven AI agent.[m
[32m+[m
[32m+[m[32m    Not a chatbot wrapper. An entity that:[m
[32m+[m[32m    - Remembers across sessions[m
[32m+[m[32m    - Learns who you are[m
[32m+[m[32m    - Acts on the ReAct loop[m
[32m+[m[32m    - Manages its own memory autonomously[m
[32m+[m
[32m+[m[32m    Usage:[m
[32m+[m[32m        agent = Agent(agent_id="my_agent", llm=VertexAIAdapter())[m
[32m+[m[32m        response = agent.interact("session_001", "What did we discuss last time?")[m
[32m+[m[32m        print(response.text)[m
[32m+[m[32m        print(response.thought.render())[m
[32m+[m[32m    """[m
[32m+[m
[32m+[m[32m    CONSOLIDATION_INTERVAL = 10  # Consolidate every N interactions[m
[32m+[m
[32m+[m[32m    def __init__([m
[32m+[m[32m        self,[m
[32m+[m[32m        agent_id:      str,[m
[32m+[m[32m        agent_type:    str = "general",[m
[32m+[m[32m        llm:           Optional[LLMAdapter] = None,[m
[32m+[m[32m        identity_dir:  str = "./identity",[m
[32m+[m[32m        data_dir:      str = "./data/agents",[m
[32m+[m[32m    ):[m
[32m+[m[32m        self.agent_id    = agent_id[m
[32m+[m[32m        self.agent_type  = agent_type[m
[32m+[m[32m        self.llm         = llm or MockAdapter()[m
[32m+[m[32m        self.identity    = load_identity(identity_dir)[m
[32m+[m
[32m+[m[32m        self.memory = MemoryEngine([m
[32m+[m[32m            agent_id  = agent_id,[m
[32m+[m[32m            storage   = LocalStorage(data_dir),[m
[32m+[m[32m            classifier= ImportanceClassifier(),[m
[32m+[m[32m        )[m
[32m+[m
[32m+[m[32m        self._interaction_count = 0[m
[32m+[m
[32m+[m[32m    # ── Core Interaction Loop ─────────────────────────────[m
[32m+[m
[32m+[m[32m    def interact(self, session_id: str, user_input: str, explicit_importance: Optional[Importance] = None) -> AgentResponse:[m
[32m+[m[32m        """[m
[32m+[m[32m        The main interaction entry point.[m
[32m+[m[32m        Executes: OBSERVE → THINK → PLAN → ACT → REFLECT → STORE[m
[32m+[m[32m        """[m
[32m+[m[32m        t0 = time.time()[m
[32m+[m[32m        trace = ThoughtTrace()[m
[32m+[m
[32m+[m[32m        # ── 1. OBSERVE ──────────────────────────────────────[m
[32m+[m[32m        trace.observation = f"New input from session '{session_id}': {user_input[:80]}..."[m
[32m+[m
[32m+[m[32m        # Store as sensory memory (replaces previous sensory)[m
[32m+[m[32m        self.memory.store(MemoryType.SENSORY, user_input, importance=Importance.LOW)[m
[32m+[m
[32m+[m[32m        # ── 2. THINK — Load memory context ──────────────────[m
[32m+[m[32m        ctx = self.memory.retrieve_context(limit_per_layer=3)[m
[32m+[m[32m        trace.memory_loaded = {k: [m.content[:60] for m in v] for k, v in ctx.items()}[m
[32m+[m
[32m+[m[32m        # ── 3. PLAN — Build prompt ───────────────────────────[m
[32m+[m[32m        prompt = self._build_prompt(user_input, session_id)[m
[32m+[m[32m        trace.plan = f"Build contextual prompt for agent_type='{self.agent_type}', using {sum(len(v) for v in ctx.values())} memory items"[m
[32m+[m
[32m+[m[32m        # ── 4. ACT — Generate response ───────────────────────[m
[32m+[m[32m        ai_response = self.llm.generate(prompt)[m
[32m+[m[32m        trace.action_taken = f"Generated response ({len(ai_response)} chars) via {self.llm.get_name()}"[m
[32m+[m
[32m+[m[32m        # ── 5. REFLECT — Score importance ────────────────────[m
[32m+[m[32m        importance = self.memory.classifier.classify([m
[32m+[m[32m            f"{user_input} {ai_response}",[m
[32m+[m[32m            explicit_importance[m
[32m+[m[32m        )[m
[32m+[m[32m        trace.importance_score = importance.value[m
[32m+[m
[32m+[m[32m        # ── 6. STORE — Commit to memory ──────────────────────[m
[32m+[m[32m        stored = [][m
[32m+[m
[32m+[m[32m        # Always store in short-term[m
[32m+[m[32m        stm = self.memory.store([m
[32m+[m[32m            MemoryType.SHORT_TERM,[m
[32m+[m[32m            f"User: {user_input}\nAgent: {ai_response}",[m
[32m+[m[32m            importance=importance,[m
[32m+[m[32m            context={"session_id": session_id},[m
[32m+[m[32m        )[m
[32m+[m[32m        stored.append(stm)[m
[32m+[m
[32m+[m[32m        # Promote to episodic if substantive[m
[32m+[m[32m        if importance in (Importance.HIGH, Importance.CRITICAL):[m
[32m+[m[32m            ep = self.memory.store([m
[32m+[m[32m                MemoryType.EPISODIC,[m
[32m+[m[32m                f"[{datetime.now().strftime('%Y-%m-%d')}] {user_input[:150]}",[m
[32m+[m[32m                importance=importance,[m
[32m+[m[32m                tags=["interaction"],[m
[32m+[m[32m                context={"session_id": session_id},[m
[32m+[m[32m            )[m
[32m+[m[32m            stored.append(ep)[m
[32m+[m
[32m+[m[32m        # Promote to long-term if critical[m
[32m+[m[32m        if importance == Importance.CRITICAL:[m
[32m+[m[32m            lt = self.memory.store([m
[32m+[m[32m                MemoryType.LONG_TERM,[m
[32m+[m[32m                f"KEY FACT: {ai_response[:200]}",[m
[32m+[m[32m                importance=Importance.CRITICAL,[m
[32m+[m[32m                tags=["promoted"],[m
[32m+[m[32m            )[m
[32m+[m[32m            stored.append(lt)[m
[32m+[m
[32m+[m[32m        trace.memories_stored = [m.id for m in stored][m
[32m+[m
[32m+[m[32m        # ── Periodic consolidation ─────────────────────────[m
[32m+[m[32m        self._interaction_count += 1[m
[32m+[m[32m        if self._interaction_count % self.CONSOLIDATION_INTERVAL == 0:[m
[32m+[m[32m            new_semantic = self.memory.consolidate()[m
[32m+[m[32m            trace.consolidation = [m.id for m in new_semantic][m
[32m+[m
[32m+[m[32m        trace.duration_ms = (time.time() - t0) * 1000[m
[32m+[m
[32m+[m[32m        return AgentResponse([m
[32m+[m[32m            agent_id     = self.agent_id,[m
[32m+[m[32m            session_id   = session_id,[m
[32m+[m[32m            text         = ai_response,[m
[32m+[m[32m            thought      = trace,[m
[32m+[m[32m            memory_stats = self.memory.stats(),[m
[32m+[m[32m        )[m
[32m+[m
[32m+[m[32m    # ── Prompt Construction ───────────────────────────────[m
[32m+[m
[32m+[m[32m    def _build_prompt(self, user_input: str, session_id: str) -> str:[m
[32m+[m[32m        """[m
[32m+[m[32m        Builds the full prompt with identity + memory context.[m
[32m+[m[32m        The identity layer is what separates this from a generic LLM call.[m
[32m+[m[32m        """[m
[32m+[m[32m        soul_excerpt = self._extract_soul_essence()[m
[32m+[m[32m        memory_context = self.memory.format_context_for_prompt(limit_per_layer=3)[m
[32m+[m[32m        agent_role = self._get_role_description()[m
[32m+[m
[32m+[m[32m        return f"""{agent_role}[m
[32m+[m
[32m+[m[32m━━━ YOUR IDENTITY ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━[m
[32m+[m[32m{soul_excerpt}[m
[32m+[m
[32m+[m[32m━━━ YOUR MEMORY ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━[m
[32m+[m[32m{memory_context}[m
[32m+[m
[32m+[m[32m━━━ OPERATING PRINCIPLES ━━━━━━━━━━━━━━━━━━━━━━━[m
[32m+[m[32m- Use memory context before asking clarifying questions[m
[32m+[m[32m- Lead with the answer, then explain[m
[32m+[m[32m- Be direct. No padding or performative enthusiasm[m
[32m+[m[32m- If you're uncertain, say so clearly[m
[32m+[m[32m- Match the language and formality of the user[m
[32m+[m[32m- Session: {session_id}[m
[32m+[m
[32m+[m[32m━━━ USER INPUT ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━[m
[32m+[m[32m{user_input}[m
[32m+[m
[32m+[m[32m━━━ RESPONSE ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━[m
[32m+[m[32m"""[m
[32m+[m
[32m+[m[32m    def _get_role_description(self) -> str:[m
[32m+[m[32m        roles = {[m
[32m+[m[32m            "general":    "You are an adaptive AI agent with dynamic memory and autonomous reasoning.",[m
[32m+[m[32m            "analyst":    "You are a data analyst agent. You find patterns, validate assumptions, and produce structured insights.",[m
[32m+[m[32m            "researcher": "You are a research agent. You synthesize information from multiple sources into coherent knowledge.",[m
[32m+[m[32m            "assistant":  "You are a contextual assistant. You remember what matters and act on it.",[m
[32m+[m[32m            "specialist": "You are a domain specialist. Deep knowledge, precise answers, no generalities.",[m
[32m+[m[32m        }[m
[32m+[m[32m        return roles.get(self.agent_type, roles["general"])[m
[32m+[m
[32m+[m[32m    def _extract_soul_essence(self) -> str:[m
[32m+[m[32m        """Extract key principles from SOUL.md for prompt injection."""[m
[32m+[m[32m        soul = self.identity.get("soul", "")[m
[32m+[m[32m        if not soul:[m
[32m+[m[32m            return "You are an autonomous agent with memory and judgment."[m
[32m+[m[32m        # Extract just the philosophy section for token efficiency[m
[32m+[m[32m        lines = soul.split("\n")[m
[32m+[m[32m        relevant = [][m
[32m+[m[32m        in_section = False[m
[32m+[m[32m        for line in lines:[m
[32m+[m[32m            if "## Philosophy" in line or "## Temperament" in line:[m
[32m+[m[32m                in_section = True[m
[32m+[m[32m            elif line.startswith("## ") and in_section:[m
[32m+[m[32m                in_section = False[m
[32m+[m[32m            if in_section and line.strip() and not line.startswith("#"):[m
[32m+[m[32m                relevant.append(line.strip())[m
[32m+[m[32m        return "\n".join(relevant[:10]) if relevant else "Curious. Persistent. Direct. Honest."[m
[32m+[m
[32m+[m[32m    # ── Memory Management API ─────────────────────────────[m
[32m+[m
[32m+[m[32m    def remember(self, fact: str, memory_type: MemoryType = MemoryType.LONG_TERM) -> Memory:[m
[32m+[m[32m        """Explicitly store a fact with CRITICAL importance."""[m
[32m+[m[32m        return self.memory.store(memory_type, fact, importance=Importance.CRITICAL, tags=["explicit"])[m
[32m+[m
[32m+[m[32m    def forget(self, memory_id: str) -> bool:[m
[32m+[m[32m        """Forget a specific memory by ID."""[m
[32m+[m[32m        return self.memory.forget(memory_id)[m
[32m+[m
[32m+[m[32m    def search_memory(self, query: str, top_k: int = 5) -> list:[m
[32m+[m[32m        """Search memory for relevant entries."""[m
[32m+[m[32m        return self.memory.search(query, top_k=top_k)[m
[32m+[m
[32m+[m[32m    def new_session(self, clear_sensory: bool = True):[m
[32m+[m[32m        """Start a fresh session while preserving long-term memory."""[m
[32m+[m[32m        self.memory.clear_session(protect_long_term=True)[m
[32m+[m
[32m+[m[32m    def stats(self) -> dict:[m
[32m+[m[32m        """Memory statistics."""[m
[32m+[m[32m        return self.memory.stats()[m
[32m+[m
[32m+[m[32m    def __repr__(self) -> str:[m
[32m+[m[32m        return f"Agent(id={self.agent_id}, type={self.agent_type}, llm={self.llm.get_name()}, {self.memory})"[m
[1mdiff --git a/memory_engine.py b/memory_engine.py[m
[1mnew file mode 100644[m
[1mindex 0000000..238da08[m
[1m--- /dev/null[m
[1m+++ b/memory_engine.py[m
[36m@@ -0,0 +1,584 @@[m
[32m+[m[32m"""[m
[32m+[m[32mcore/memory_engine.py[m
[32m+[m
[32m+[m[32mThe Memory Engine — autonomous memory management for AI agents.[m
[32m+[m[32mThis is not a service layer. This is a cognitive architecture.[m
[32m+[m
[32m+[m[32mEach agent has a MemoryEngine instance that manages all five memory types,[m
[32m+[m[32mruns the importance classifier, handles pruning, and performs consolidation.[m
[32m+[m[32m"""[m
[32m+[m
[32m+[m[32mimport uuid[m
[32m+[m[32mimport json[m
[32m+[m[32mimport os[m
[32m+[m[32mfrom datetime import datetime, timedelta[m
[32m+[m[32mfrom typing import Optional[m
[32m+[m[32mfrom pathlib import Path[m
[32m+[m[32mfrom dataclasses import dataclass, field, asdict[m
[32m+[m[32mfrom enum import Enum[m
[32m+[m[32mfrom sentence_transformers import SentenceTransformer[m
[32m+[m[32mfrom sentence_transformers.util import cos_sim[m
[32m+[m[32mimport torch[m
[32m+[m
[32m+[m
[32m+[m[32m# ─────────────────────────────────────────[m
[32m+[m[32m# TYPES[m
[32m+[m[32m# ─────────────────────────────────────────[m
[32m+[m
[32m+[m[32mclass MemoryType(str, Enum):[m
[32m+[m[32m    SENSORY    = "sensory"      # Current turn only — immediate context[m
[32m+[m[32m    SHORT_TERM = "short_term"   # Session-scoped — recent exchanges[m
[32m+[m[32m    LONG_TERM  = "long_term"    # Persistent — important facts[m
[32m+[m[32m    EPISODIC   = "episodic"     # Past experiences — what happened[m
[32m+[m[32m    SEMANTIC   = "semantic"     # Learned concepts — what was extracted[m
[32m+[m
[32m+[m[32mclass Importance(str, Enum):[m
[32m+[m[32m    CRITICAL = "critical"   # Score: 4 — Never prune[m
[32m+[m[32m    HIGH     = "high"       # Score: 3 — Prune only under extreme pressure[m
[32m+[m[32m    MEDIUM   = "medium"     # Score: 2 — Prune when needed[m
[32m+[m[32m    LOW      = "low"        # Score: 1 — Prune first[m
[32m+[m
[32m+[m
[32m+[m[32mIMPORTANCE_SCORE = {[m
[32m+[m[32m    Importance.CRITICAL: 4,[m
[32m+[m[32m    Importance.HIGH:     3,[m
[32m+[m[32m    Importance.MEDIUM:   2,[m
[32m+[m[32m    Importance.LOW:      1,[m
[32m+[m[32m}[m
[32m+[m
[32m+[m[32m# Memory layer capacity limits[m
[32m+[m[32mLAYER_LIMITS = {[m
[32m+[m[32m    MemoryType.SENSORY:    1,[m
[32m+[m[32m    MemoryType.SHORT_TERM: 20,[m
[32m+[m[32m    MemoryType.LONG_TERM:  500,[m
[32m+[m[32m    MemoryType.EPISODIC:   200,[m
[32m+[m[32m    MemoryType.SEMANTIC:   1000,[m
[32m+[m[32m}[m
[32m+[m
[32m+[m[32m# Long-term layer threshold — only HIGH+ goes here automatically[m
[32m+[m[32mLONG_TERM_THRESHOLD = Importance.HIGH[m
[32m+[m
[32m+[m
[32m+[m[32m# ─────────────────────────────────────────[m
[32m+[m[32m# MEMORY UNIT[m
[32m+[m[32m# ─────────────────────────────────────────[m
[32m+[m
[32m+[m[32m@dataclass[m
[32m+[m[32mclass Memory:[m
[32m+[m[32m    """[m
[32m+[m[32m    The atomic unit of agent memory.[m
[32m+[m[32m    Not just a dict. A typed, scored, timestamped cognitive entry.[m
[32m+[m[32m    """[m
[32m+[m[32m    id:           str[m
[32m+[m[32m    type:         MemoryType[m
[32m+[m[32m    content:      str[m
[32m+[m[32m    importance:   Importance       = Importance.MEDIUM[m
[32m+[m[32m    access_count: int              = 0[m
[32m+[m[32m    created_at:   str              = field(default_factory=lambda: datetime.now().isoformat())[m
[32m+[m[32m    last_accessed: Optional[str]   = None[m
[32m+[m[32m    tags:         list             = field(default_factory=list)[m
[32m+[m[32m    context:      dict             = field(default_factory=dict)[m
[32m+[m
[32m+[m[32m    def touch(self):[m
[32m+[m[32m        """Record that this memory was accessed."""[m
[32m+[m[32m        self.access_count += 1[m
[32m+[m[32m        self.last_accessed = datetime.now().isoformat()[m
[32m+[m
[32m+[m[32m    def score(self) -> float:[m
[32m+[m[32m        """[m
[32m+[m[32m        Composite retention score.[m
[32m+[m[32m        Higher = more valuable to keep.[m
[32m+[m[32m        Used by pruner to decide what to forget.[m
[32m+[m[32m        """[m
[32m+[m[32m        importance_weight = IMPORTANCE_SCORE.get(self.importance, 1)[m
[32m+[m[32m        recency_bonus = 0[m
[32m+[m[32m        if self.last_accessed:[m
[32m+[m[32m            hours_since = (datetime.now() - datetime.fromisoformat(self.last_accessed)).total_seconds() / 3600[m
[32m+[m[32m            recency_bonus = max(0, 1 - (hours_since / 72))  # Decays over 72h[m
[32m+[m[32m        return (importance_weight * 2) + self.access_count + recency_bonus[m
[32m+[m
[32m+[m[32m    def to_dict(self) -> dict:[m
[32m+[m[32m        return asdict(self)[m
[32m+[m
[32m+[m[32m    @classmethod[m
[32m+[m[32m    def from_dict(cls, d: dict) -> "Memory":[m
[32m+[m[32m        d["type"]       = MemoryType(d["type"])[m
[32m+[m[32m        d["importance"] = Importance(d["importance"])[m
[32m+[m[32m        return cls(**d)[m
[32m+[m
[32m+[m
[32m+[m[32m# ─────────────────────────────────────────[m
[32m+[m[32m# IMPORTANCE CLASSIFIER[m
[32m+[m[32m# ─────────────────────────────────────────[m
[32m+[m
[32m+[m[32mclass ImportanceClassifier:[m
[32m+[m[32m    """[m
[32m+[m[32m    Autonomous importance scoring.[m
[32m+[m[32m    The agent decides what matters — user doesn't have to tag everything.[m
[32m+[m[41m    [m
[32m+[m[32m    Extensible: override `classify()` to plug in an LLM-based scorer.[m
[32m+[m[32m    """[m
[32m+[m
[32m+[m[32m    CRITICAL_SIGNALS = [[m
[32m+[m[32m        "remember this", "never forget", "critical", "crucial", "vital",[m
[32m+[m[32m        "always", "must", "required", "do not", "don't ever",[m
[32m+[m[32m    ][m
[32m+[m[32m    HIGH_SIGNALS = [[m
[32m+[m[32m        "important", "key", "significant", "main", "primary",[m
[32m+[m[32m        "prefer", "always use", "my name is", "i am", "we are",[m
[32m+[m[32m    ][m
[32m+[m[32m    LOW_SIGNALS = [[m
[32m+[m[32m        "thanks", "ok", "sure", "got it", "understood", "yes", "no",[m
[32m+[m[32m        "hello", "hi", "bye",[m
[32m+[m[32m    ][m
[32m+[m
[32m+[m[32m    def classify(self, content: str, explicit_override: Optional[Importance] = None) -> Importance:[m
[32m+[m[32m        """[m
[32m+[m[32m        Classify importance of a memory content string.[m
[32m+[m[32m        explicit_override lets the caller force a level (e.g., user said "remember this").[m
[32m+[m[32m        """[m
[32m+[m[32m        if explicit_override:[m
[32m+[m[32m            return explicit_override[m
[32m+[m
[32m+[m[32m        text = content.lower()[m
[32m+[m
[32m+[m[32m        if any(sig in text for sig in self.CRITICAL_SIGNALS):[m
[32m+[m[32m            return Importance.CRITICAL[m
[32m+[m
[32m+[m[32m        if any(sig in text for sig in self.HIGH_SIGNALS):[m
[32m+[m[32m            return Importance.HIGH[m
[32m+[m
[32m+[m[32m        if any(sig in text for sig in self.LOW_SIGNALS):[m
[32m+[m[32m            return Importance.LOW[m
[32m+[m
[32m+[m[32m        # Length heuristic — longer interactions tend to be more substantive[m
[32m+[m[32m        if len(content) > 500:[m
[32m+[m[32m            return Importance.HIGH[m
[32m+[m[32m        if len(content) > 150:[m
[32m+[m[32m            return Importance.MEDIUM[m
[32m+[m
[32m+[m[32m        return Importance.LOW[m
[32m+[m
[32m+[m
[32m+[m[32m# ─────────────────────────────────────────[m
[32m+[m[32m# STORAGE BACKEND[m
[32m+[m[32m# ─────────────────────────────────────────[m
[32m+[m
[32m+[m[32mclass LocalStorage:[m
[32m+[m[32m    """[m
[32m+[m[32m    File-based storage backend.[m
[32m+[m[32m    Saves each agent's memory as a JSON file.[m
[32m+[m[32m    Swap this for Firestore, Redis, SQLite, etc. without touching MemoryEngine.[m
[32m+[m[32m    """[m
[32m+[m
[32m+[m[32m    def __init__(self, data_dir: str = "./data/agents"):[m
[32m+[m[32m        self.data_dir = Path(data_dir)[m
[32m+[m[32m        self.data_dir.mkdir(parents=True, exist_ok=True)[m
[32m+[m
[32m+[m[32m    def _path(self, agent_id: str) -> Path:[m
[32m+[m[32m        return self.data_dir / f"{agent_id}.json"[m
[32m+[m
[32m+[m[32m    def load(self, agent_id: str) -> dict:[m
[32m+[m[32m        path = self._path(agent_id)[m
[32m+[m[32m        if not path.exists():[m
[32m+[m[32m            return {"memories": [], "meta": {}}[m
[32m+[m[32m        with open(path) as f:[m
[32m+[m[32m            return json.load(f)[m
[32m+[m
[32m+[m[32m    def save(self, agent_id: str, data: dict):[m
[32m+[m[32m        with open(self._path(agent_id), "w") as f:[m
[32m+[m[32m            json.dump(data, f, indent=2, default=str)[m
[32m+[m
[32m+[m[32m    def delete(self, agent_id: str):[m
[32m+[m[32m        path = self._path(agent_id)[m
[32m+[m[32m        if path.exists():[m
[32m+[m[32m            path.unlink()[m
[32m+[m
[32m+[m[32m    def list_agents(self) -> list[str]:[m
[32m+[m[32m        return [p.stem for p in self.data_dir.glob("*.json")][m
[32m+[m
[32m+[m
[32m+[m[32m# ─────────────────────────────────────────[m
[32m+[m[32m# MEMORY ENGINE[m
[32m+[m[32m# ─────────────────────────────────────────[m
[32m+[m
[32m+[m[32mclass MemoryEngine:[m
[32m+[m[32m    """[m
[32m+[m[32m    The cognitive core of an autonomous agent.[m
[32m+[m[41m    [m
[32m+[m[32m    Manages five memory layers, autonomously classifies importance,[m
[32m+[m[32m    prunes strategically, and consolidates episodic → semantic memory.[m
[32m+[m[41m    [m
[32m+[m[32m    This is not a CRUD layer. This is a cognitive architecture.[m
[32m+[m[41m    [m
[32m+[m[32m    Usage:[m
[32m+[m[32m        engine = MemoryEngine(agent_id="my_agent")[m
[32m+[m[32m        engine.store(MemoryType.SHORT_TERM, "User said they prefer Python 3.11")[m
[32m+[m[32m        memories = engine.retrieve(MemoryType.SHORT_TERM, limit=5)[m
[32m+[m[32m        engine.prune()[m
[32m+[m[32m    """[m
[32m+[m
[32m+[m[32m    def __init__([m
[32m+[m[32m        self,[m
[32m+[m[32m        agent_id: str,[m
[32m+[m[32m        storage: Optional[LocalStorage] = None,[m
[32m+[m[32m        classifier: Optional[ImportanceClassifier] = None,[m
[32m+[m[32m        embed_model_name: str = "all-MiniLM-L6-v2",[m
[32m+[m[32m    ):[m
[32m+[m[32m        self.agent_id   = agent_id[m
[32m+[m[32m        self.storage    = storage or LocalStorage()[m
[32m+[m[32m        self.classifier = classifier or ImportanceClassifier()[m
[32m+[m[32m        self._memories: list[Memory] = [][m
[32m+[m[32m        self._meta: dict = {}[m
[32m+[m[32m        self._load()[m
[32m+[m[32m        self._embed_model_name = embed_model_name[m
[32m+[m[32m        self._embedder: SentenceTransformer | None = None[m
[32m+[m[32m        self._embeddings: torch.Tensor | None = None        # shape: (n_mem, dim)[m
[32m+[m[32m        self._embed_index_to_mem_index: list[int] | None = None # it # maps embedding row --> self._memories index[m
[32m+[m[41m        [m
[32m+[m[32m        @property[m
[32m+[m[32m        def embedder(self) -> SentenceTransformer:[m
[32m+[m[32m            if self._embedder is None:[m
[32m+[m[32m                self._embedder = SentenceTransformer(self._embed_model_name)[m
[32m+[m[32m                #if torch.cuda.is_available(): self._embedder = self._embedder.cuda() #moves to GPU if available and desired[m
[32m+[m[32m                self._embedder = SentenceTransformer(self._embed_model_name)[m
[32m+[m[32m            return self._embedder[m
[32m+[m[41m        [m
[32m+[m[32m        def _rebuild_embedding_cache(self) -> None:[m
[32m+[m[32m            if not self._memories:[m
[32m+[m[32m                self._embeddings = None[m
[32m+[m[32m                self._embed_index_to_mem_index = None[m
[32m+[m[32m                return[m
[32m+[m
[32m+[m[32m            contents = [m.content.strip() for m in self._memories if m.content.strip()][m
[32m+[m[32m            if not contents:[m
[32m+[m[32m                self._embeddings = None[m
[32m+[m[32m                return[m
[32m+[m
[32m+[m[32m            emb_list = self.embedder.encode([m
[32m+[m[32m                contents,[m
[32m+[m[32m                batch_size=32,[m
[32m+[m[32m                show_progress_bar=False,[m
[32m+[m[32m                convert_to_tensor=True,[m
[32m+[m[32m                device=self.embedder.device[m
[32m+[m[32m            )[m
[32m+[m
[32m+[m[32m            self._embeddings = emb_list[m
[32m+[m[32m            # Keep mapping because we might have empty contents skipped (rare)[m
[32m+[m[32m            self._embed_index_to_mem_index = list(range(len(self._memories)))[m
[32m+[m
[32m+[m[32m            def _ensure_embeddings(self):[m
[32m+[m[32m                if self._embeddings is None or len(self._embeddings) != len(self._memories):[m
[32m+[m[32m                    self._rebuild_embedding_cache()[m
[32m+[m
[32m+[m
[32m+[m[32m    # ── Persistence ──────────────────────────────────────[m
[32m+[m
[32m+[m[32m    def _load(self):[m
[32m+[m[32m        data = self.storage.load(self.agent_id)[m
[32m+[m[32m        self._memories = [Memory.from_dict(m) for m in data.get("memories", [])][m
[32m+[m[32m        self._meta = data.get("meta", {[m
[32m+[m[32m            "agent_id": self.agent_id,[m
[32m+[m[32m            "created_at": datetime.now().isoformat(),[m
[32m+[m[32m            "total_interactions": 0,[m
[32m+[m[32m        })[m
[32m+[m
[32m+[m[32m    def _save(self):[m
[32m+[m[32m        self.storage.save(self.agent_id, {[m
[32m+[m[32m            "memories": [m.to_dict() for m in self._memories],[m
[32m+[m[32m            "meta": self._meta,[m
[32m+[m[32m        })[m
[32m+[m
[32m+[m[32m    # ── Core Operations ───────────────────────────────────[m
[32m+[m
[32m+[m[32m    def store([m
[32m+[m[32m        self,[m
[32m+[m[32m        memory_type: MemoryType,[m
[32m+[m[32m        content: str,[m
[32m+[m[32m        importance: Optional[Importance] = None,[m
[32m+[m[32m        tags: list = None,[m
[32m+[m[32m        context: dict = None,[m
[32m+[m[32m        explicit_importance: Optional[Importance] = None,[m
[32m+[m[32m    ) -> Memory:[m
[32m+[m[32m        """[m
[32m+[m[32m        Store a memory. Auto-classifies importance if not provided.[m
[32m+[m[41m        [m
[32m+[m[32m        For SENSORY memory: replaces the single existing sensory entry.[m
[32m+[m[32m        For SHORT_TERM: appends and prunes if over limit.[m
[32m+[m[32m        For LONG_TERM: only stores if importance >= HIGH.[m
[32m+[m[32m        """[m
[32m+[m[32m        resolved_importance = importance or self.classifier.classify(content, explicit_importance)[m
[32m+[m
[32m+[m[32m        # LONG_TERM has a quality gate[m
[32m+[m[32m        if memory_type == MemoryType.LONG_TERM:[m
[32m+[m[32m            if IMPORTANCE_SCORE.get(resolved_importance, 0) < IMPORTANCE_SCORE[LONG_TERM_THRESHOLD]:[m
[32m+[m[32m                # Redirect LOW/MEDIUM to SHORT_TERM instead[m
[32m+[m[32m                memory_type = MemoryType.SHORT_TERM[m
[32m+[m
[32m+[m[32m        mem = Memory([m
[32m+[m[32m            id          = str(uuid.uuid4()),[m
[32m+[m[32m            type        = memory_type,[m
[32m+[m[32m            content     = content,[m
[32m+[m[32m            importance  = resolved_importance,[m
[32m+[m[32m            tags        = tags or [],[m
[32m+[m[32m            context     = context or {},[m
[32m+[m[32m        )[m
[32m+[m
[32m+[m[32m        # SENSORY: single slot — replace existing[m
[32m+[m[32m        if memory_type == MemoryType.SENSORY:[m
[32m+[m[32m            self._memories = [m for m in self._memories if m.type != MemoryType.SENSORY][m
[32m+[m
[32m+[m[32m        self._memories.append(mem)[m
[32m+[m[32m        self._prune_layer(memory_type)[m
[32m+[m[32m        self._save()[m
[32m+[m[32m        return mem[m
[32m+[m
[32m+[m[32m    def retrieve([m
[32m+[m[32m        self,[m
[32m+[m[32m        memory_type: Optional[MemoryType] = None,[m
[32m+[m[32m        limit: int = 10,[m
[32m+[m[32m        query: Optional[str] = None,[m
[32m+[m[32m    ) -> list[Memory]:[m
[32m+[m[32m        """[m
[32m+[m[32m        Retrieve memories by type. Optional simple text filter.[m
[32m+[m[32m        Returns sorted by score (most valuable first).[m
[32m+[m[32m        """[m
[32m+[m[32m        results = self._memories[m
[32m+[m
[32m+[m[32m        if memory_type:[m
[32m+[m[32m            results = [m for m in results if m.type == memory_type][m
[32m+[m
[32m+[m[32m        if query:[m
[32m+[m[32m            q = query.lower()[m
[32m+[m[32m            results = [m for m in results if q in m.content.lower()][m
[32m+[m
[32m+[m[32m        # Sort by composite score[m
[32m+[m[32m        results = sorted(results, key=lambda m: m.score(), reverse=True)[m
[32m+[m
[32m+[m[32m        # Touch accessed memories (update recency)[m
[32m+[m[32m        for m in results[:limit]:[m
[32m+[m[32m            m.touch()[m
[32m+[m
[32m+[m[32m        self._save()[m
[32m+[m[32m        return results[:limit][m
[32m+[m[41m        [m
[32m+[m[32m    def retrieve_context(self, limit_per_layer: int = 3) -> dict[str, list[Memory]]:[m
[32m+[m[32m        """[m
[32m+[m[32m        Retrieve a balanced context snapshot across all memory layers.[m
[32m+[m[32m        Used by the agent when building its prompt context.[m
[32m+[m[32m        """[m
[32m+[m[32m        return {[m
[32m+[m[32m            "sensory":    self.retrieve(MemoryType.SENSORY,    limit=1),[m
[32m+[m[32m            "short_term": self.retrieve(MemoryType.SHORT_TERM, limit=limit_per_layer),[m
[32m+[m[32m            "long_term":  self.retrieve(MemoryType.LONG_TERM,  limit=limit_per_layer),[m
[32m+[m[32m            "episodic":   self.retrieve(MemoryType.EPISODIC,   limit=limit_per_layer),[m
[32m+[m[32m            "semantic":   self.retrieve(MemoryType.SEMANTIC,   limit=limit_per_layer),[m
[32m+[m[32m        }[m
[32m+[m[41m        [m
[32m+[m[32m        #work here[m
[32m+[m[32m    def search(self, query: str, top_k: int = 5, min_score_threshold: float = 0.3) -> list[Memory]:[m
[32m+[m[32m            """[m
[32m+[m[32m            Semantic search across all memories using sentence-transformers embeddings.[m
[32m+[m[32m            Falls back to substring search if embedding model is disabled/not loaded.[m
[32m+[m[32m            Update: Similarity search added with a minimum score threshold and fallback to substring search if embedding fails.[m
[32m+[m[32m            """[m
[32m+[m[32m            if not query.strip() or not self._memories:[m
[32m+[m[32m                return [][m
[32m+[m
[32m+[m[32m            query = query.strip()[m
[32m+[m[32m            q_lower = query.lower()[m
[32m+[m
[32m+[m[32m            use_semantic = self._embed_model_name is not None[m
[32m+[m
[32m+[m[32m            if use_semantic:[m
[32m+[m[32m                try:[m
[32m+[m[32m                    self._ensure_embeddings()[m
[32m+[m
[32m+[m[32m                    if self._embeddings is not None and self._embeddings.shape[0] > 0:[m
[32m+[m[32m                        q_emb = self.embedder.encode(query, convert_to_tensor=True)[m
[32m+[m[32m                        sims = cos_sim(q_emb.unsqueeze(0), self._embeddings)[0]   # shape (n_mem,)[m
[32m+[m
[32m+[m[32m                        # Get top candidates[m
[32m+[m[32m                        values, indices = torch.topk(sims, k=min(top_k * 3, len(sims)), sorted=True)[m
[32m+[m
[32m+[m[32m                        result_memories = [][m
[32m+[m[32m                        for val, idx in zip(values.tolist(), indices.tolist()):[m
[32m+[m[32m                            if val.item() < min_score_threshold:[m
[32m+[m[32m                                break[m
[32m+[m[32m                            mem_idx = self._embed_index_to_mem_index[idx][m
[32m+[m[32m                            mem = self._memories[mem_idx][m
[32m+[m[32m                            mem._semantic_score = val.item()   # optional: expose for debugging[m
[32m+[m[32m                            result_memories.append(mem)[m
[32m+[m
[32m+[m[32m                        # Still sort by your original composite score as secondary key[m
[32m+[m[32m                        result_memories.sort(key=lambda m: (getattr(m, '_semantic_score', 0), m.score()), reverse=True)[m
[32m+[m[32m                        return result_memories[:top_k][m
[32m+[m
[32m+[m[32m                except Exception as e:[m
[32m+[m[32m                    print(f"Semantic search failed: {e} → falling back to substring")[m
[32m+[m[32m                    # continue to fallback[m
[32m+[m
[32m+[m[32m            # Fallback: original substring + score sort[m
[32m+[m[32m            results = [m for m in self._memories if q_lower in m.content.lower()][m
[32m+[m[32m            results = sorted(results, key=lambda m: m.score(), reverse=True)[:top_k][m
[32m+[m[32m            return results[m
[32m+[m
[32m+[m[32m    def forget(self, memory_id: str) -> bool:[m
[32m+[m[32m        """Explicitly forget a specific memory by ID."""[m
[32m+[m[32m        before = len(self._memories)[m
[32m+[m[32m        self._memories = [m for m in self._memories if m.id != memory_id][m
[32m+[m[32m        if len(self._memories) < before:[m
[32m+[m[32m            self._save()[m
[32m+[m[32m            return True[m
[32m+[m[32m        return False[m
[32m+[m
[32m+[m[32m    def clear_type(self, memory_type: MemoryType, protect_critical: bool = True):[m
[32m+[m[32m        """Clear all memories of a given type. Optionally protect CRITICAL ones."""[m
[32m+[m[32m        def should_keep(m: Memory) -> bool:[m
[32m+[m[32m            if m.type != memory_type:[m
[32m+[m[32m                return True[m
[32m+[m[32m            if protect_critical and m.importance == Importance.CRITICAL:[m
[32m+[m[32m                return True[m
[32m+[m[32m            return False[m
[32m+[m[32m        self._memories = [m for m in self._memories if should_keep(m)][m
[32m+[m[32m        self._save()[m
[32m+[m
[32m+[m[32m    def clear_session(self, protect_long_term: bool = True):[m
[32m+[m[32m        """[m
[32m+[m[32m        Clear session-scoped memories (sensory + short_term).[m
[32m+[m[32m        Long-term, episodic, and semantic survive by default.[m
[32m+[m[32m        """[m
[32m+[m[32m        session_types = {MemoryType.SENSORY, MemoryType.SHORT_TERM}[m
[32m+[m[32m        if protect_long_term:[m
[32m+[m[32m            self._memories = [m for m in self._memories if m.type not in session_types][m
[32m+[m[32m        else:[m
[32m+[m[32m            self._memories = [][m
[32m+[m[32m        self._save()[m
[32m+[m
[32m+[m[32m    # ── Pruning ───────────────────────────────────────────[m
[32m+[m
[32m+[m[32m    def _prune_layer(self, memory_type: MemoryType):[m
[32m+[m[32m        """[m
[32m+[m[32m        If a layer exceeds its limit, prune the lowest-scoring memories.[m
[32m+[m[32m        CRITICAL memories are always protected.[m
[32m+[m[32m        """[m
[32m+[m[32m        layer = [m for m in self._memories if m.type == memory_type][m
[32m+[m[32m        limit = LAYER_LIMITS.get(memory_type, 100)[m
[32m+[m
[32m+[m[32m        if len(layer) <= limit:[m
[32m+[m[32m            return[m
[32m+[m
[32m+[m[32m        # Protect critical[m
[32m+[m[32m        protected = [m for m in layer if m.importance == Importance.CRITICAL][m
[32m+[m[32m        candidates = [m for m in layer if m.importance != Importance.CRITICAL][m
[32m+[m
[32m+[m[32m        # Keep top N by score[m
[32m+[m[32m        keep = limit - len(protected)[m
[32m+[m[32m        survivors = sorted(candidates, key=lambda m: m.score(), reverse=True)[:max(0, keep)][m
[32m+[m
[32m+[m[32m        keep_ids = {m.id for m in protected + survivors}[m
[32m+[m[32m        self._memories = [m for m in self._memories if m.type != memory_type or m.id in keep_ids][m
[32m+[m
[32m+[m[32m    def prune(self):[m
[32m+[m[32m        """Run pruning across all layers."""[m
[32m+[m[32m        for mt in MemoryType:[m
[32m+[m[32m            self._prune_layer(mt)[m
[32m+[m[32m        self._save()[m
[32m+[m
[32m+[m[32m    # ── Consolidation (Dream-like processing) ─────────────[m
[32m+[m
[32m+[m[32m    def consolidate(self) -> list[Memory]:[m
[32m+[m[32m        """[m
[32m+[m[32m        Episodic → Semantic consolidation.[m
[32m+[m[41m        [m
[32m+[m[32m        Takes highly-accessed episodic memories and extracts[m
[32m+[m[32m        generalized concepts into semantic memory.[m
[32m+[m[41m        [m
[32m+[m[32m        This mimics how humans consolidate experiences into knowledge during sleep.[m
[32m+[m[32m        Returns list of newly created semantic memories.[m
[32m+[m[32m        """[m
[32m+[m[32m        new_semantic: list[Memory] = [][m
[32m+[m
[32m+[m[32m        candidates = [[m
[32m+[m[32m            m for m in self._memories[m
[32m+[m[32m            if m.type == MemoryType.EPISODIC[m
[32m+[m[32m            and m.access_count >= 3[m
[32m+[m[32m            and IMPORTANCE_SCORE.get(m.importance, 0) >= 2[m
[32m+[m[32m        ][m
[32m+[m
[32m+[m[32m        for ep in candidates:[m
[32m+[m[32m            # Create a semantic memory from this episodic one[m
[32m+[m[32m            concept = f"[Consolidated from experience] {ep.content[:300]}"[m
[32m+[m
[32m+[m[32m            # Check if we already have something similar (simple check)[m
[32m+[m[32m            existing = self.search(ep.content[:50])[m
[32m+[m[32m            similar_semantic = [m for m in existing if m.type == MemoryType.SEMANTIC][m
[32m+[m[32m            if similar_semantic:[m
[32m+[m[32m                continue  # Already consolidated[m
[32m+[m
[32m+[m[32m            sem = self.store([m
[32m+[m[32m                memory_type=MemoryType.SEMANTIC,[m
[32m+[m[32m                content=concept,[m
[32m+[m[32m                importance=Importance.HIGH,[m
[32m+[m[32m                tags=["consolidated", "from_episodic"],[m
[32m+[m[32m                context={"source_episodic_id": ep.id},[m
[32m+[m[32m            )[m
[32m+[m[32m            new_semantic.append(sem)[m
[32m+[m
[32m+[m[32m        return new_semantic[m
[32m+[m
[32m+[m[32m    # ── Introspection ────────────────────────────────────[m
[32m+[m
[32m+[m[32m    def stats(self) -> dict:[m
[32m+[m[32m        """Memory statistics snapshot."""[m
[32m+[m[32m        counts = {mt.value: 0 for mt in MemoryType}[m
[32m+[m[32m        importance_dist = {imp.value: 0 for imp in Importance}[m
[32m+[m
[32m+[m[32m        for m in self._memories:[m
[32m+[m[32m            counts[m.type.value] += 1[m
[32m+[m[32m            importance_dist[m.importance.value] += 1[m
[32m+[m
[32m+[m[32m        total = len(self._memories)[m
[32m+[m[32m        capacity = sum(LAYER_LIMITS.values())[m
[32m+[m
[32m+[m[32m        return {[m
[32m+[m[32m            "agent_id":         self.agent_id,[m
[32m+[m[32m            "total_memories":   total,[m
[32m+[m[32m            "memory_pressure":  round(total / capacity, 3),[m
[32m+[m[32m            "by_type":          counts,[m
[32m+[m[32m            "by_importance":    importance_dist,[m
[32m+[m[32m            "total_capacity":   capacity,[m
[32m+[m[32m        }[m
[32m+[m
[32m+[m[32m    def format_context_for_prompt(self, limit_per_layer: int = 3) -> str:[m
[32m+[m[32m        """[m
[32m+[m[32m        Format memory context for inclusion in an LLM prompt.[m
[32m+[m[32m        Returns a structured string the agent uses to recall context.[m
[32m+[m[32m        """[m
[32m+[m[32m        ctx = self.retrieve_context(limit_per_layer=limit_per_layer)[m
[32m+[m[32m        lines = [][m
[32m+[m
[32m+[m[32m        label_map = {[m
[32m+[m[32m            "sensory":    "[ CURRENT CONTEXT ]",[m
[32m+[m[32m            "short_term": "[ RECENT INTERACTIONS ]",[m
[32m+[m[32m            "long_term":  "[ IMPORTANT FACTS ]",[m
[32m+[m[32m            "episodic":   "[ PAST EXPERIENCES ]",[m
[32m+[m[32m            "semantic":   "[ LEARNED KNOWLEDGE ]",[m
[32m+[m[32m        }[m
[32m+[m
[32m+[m[32m        for layer, memories in ctx.items():[m
[32m+[m[32m            if memories:[m
[32m+[m[32m                lines.append(label_map[layer])[m
[32m+[m[32m                for m in memories:[m
[32m+[m[32m                    lines.append(f"  • {m.content}")[m
[32m+[m[32m                lines.append("")[m
[32m+[m
[32m+[m[32m        return "\n".join(lines) if lines else "[ No memory context available ]"[m
[32m+[m
[32m+[m[32m    def __repr__(self) -> str:[m
[32m+[m[32m        s = self.stats()[m
[32m+[m[32m        return ([m
[32m+[m[32m            f"MemoryEngine(agent={self.agent_id}, "[m
[32m+[m[32m            f"memories={s['total_memories']}, "[m
[32m+[m[32m            f"pressure={s['memory_pressure']:.1%})"[m
[32m+[m[32m        )[m
[1mdiff --git a/metrics.py b/metrics.py[m
[1mnew file mode 100644[m
[1mindex 0000000..876f00c[m
[1m--- /dev/null[m
[1m+++ b/metrics.py[m
[36m@@ -0,0 +1,216 @@[m
[32m+[m[32m"""[m
[32m+[m[32mcore/metrics.py[m
[32m+[m
[32m+[m[32mReal-time reliability metrics — the data an agent scoring function needs[m
[32m+[m[32mto decide whether to use Agent Memory.[m
[32m+[m
[32m+[m[32mGary Tan's three factors:[m
[32m+[m[32m  1. Liveness   — is it responding right now?[m
[32m+[m[32m  2. Reliability — has it worked before?[m
[32m+[m[32m  3. Confidence  — how often does it return accurate results?[m
[32m+[m
[32m+[m[32mThis module tracks all three in real time and exposes them[m
[32m+[m[32mas machine-readable endpoints.[m
[32m+[m[32m"""[m
[32m+[m
[32m+[m[32mimport time[m
[32m+[m[32mimport json[m
[32m+[m[32mfrom datetime import datetime, timedelta[m
[32m+[m[32mfrom dataclasses import dataclass, field, asdict[m
[32m+[m[32mfrom pathlib import Path[m
[32m+[m[32mfrom threading import Lock[m
[32m+[m[32mfrom typing import Optional[m
[32m+[m
[32m+[m
[32m+[m[32m# ─────────────────────────────────────────[m
[32m+[m[32m# DATA STRUCTURES[m
[32m+[m[32m# ─────────────────────────────────────────[m
[32m+[m
[32m+[m[32m@dataclass[m
[32m+[m[32mclass OperationRecord:[m
[32m+[m[32m    """Single operation outcome record."""[m
[32m+[m[32m    timestamp:    float[m
[32m+[m[32m    operation:    str      # interact | store | retrieve | consolidate[m
[32m+[m[32m    success:      bool[m
[32m+[m[32m    duration_ms:  float[m
[32m+[m[32m    agent_id:     str[m
[32m+[m[32m    error:        Optional[str] = None[m
[32m+[m
[32m+[m
[32m+[m[32m@dataclass[m
[32m+[m[32mclass MetricsSnapshot:[m
[32m+[m[32m    """Point-in-time reliability snapshot — what an agent scoring function reads."""[m
[32m+[m[32m    timestamp:             str[m
[32m+[m[32m    liveness:              bool     # Is the service responding right now?[m
[32m+[m[32m    uptime_pct_24h:        float    # Uptime last 24h (0.0 - 1.0)[m
[32m+[m[32m    uptime_pct_30d:        float    # Uptime last 30d (0.0 - 1.0)[m
[32m+[m[32m    success_rate_1h:       float    # Success rate last hour[m
[32m+[m[32m    success_rate_24h:      float    # Success rate last 24h[m
[32m+[m[32m    latency_p50_ms:        float    # Median latency[m
[32m+[m[32m    latency_p95_ms:        float    # 95th percentile latency[m
[32m+[m[32m    latency_p99_ms:        float    # 99th percentile latency[m
[32m+[m[32m    total_operations:      int      # All time[m
[32m+[m[32m    operations_last_1h:    int[m
[32m+[m[32m    operations_last_24h:   int[m
[32m+[m[32m    confidence_score:      float    # Composite 0.0 - 1.0[m
[32m+[m[32m    active_agents:         int[m
[32m+[m[32m    total_memories_stored: int[m
[32m+[m
[32m+[m
[32m+[m[32m# ─────────────────────────────────────────[m
[32m+[m[32m# METRICS COLLECTOR[m
[32m+[m[32m# ─────────────────────────────────────────[m
[32m+[m
[32m+[m[32mclass MetricsCollector:[m
[32m+[m[32m    """[m
[32m+[m[32m    Thread-safe metrics collector.[m
[32m+[m[32m    Wraps around Agent Memory operations to track outcomes.[m
[32m+[m
[32m+[m[32m    Usage:[m
[32m+[m[32m        metrics = MetricsCollector()[m
[32m+[m
[32m+[m[32m        # Wrap any operation[m
[32m+[m[32m        with metrics.track("interact", agent_id="my_agent"):[m
[32m+[m[32m            result = agent.interact(...)[m
[32m+[m
[32m+[m[32m        # Get snapshot[m
[32m+[m[32m        snapshot = metrics.snapshot()[m
[32m+[m[32m    """[m
[32m+[m
[32m+[m[32m    MAX_RECORDS = 10_000  # Rolling window[m
[32m+[m
[32m+[m[32m    def __init__(self, persist_path: str = "./data/metrics.json"):[m
[32m+[m[32m        self._records: list[OperationRecord] = [][m
[32m+[m[32m        self._lock = Lock()[m
[32m+[m[32m        self._start_time = time.time()[m
[32m+[m[32m        self._persist_path = Path(persist_path)[m
[32m+[m[32m        self._persist_path.parent.mkdir(parents=True, exist_ok=True)[m
[32m+[m[32m        self._load()[m
[32m+[m
[32m+[m[32m    def record(self, operation: str, agent_id: str, success: bool, duration_ms: float, error: str = None):[m
[32m+[m[32m        """Record an operation outcome."""[m
[32m+[m[32m        rec = OperationRecord([m
[32m+[m[32m            timestamp   = time.time(),[m
[32m+[m[32m            operation   = operation,[m
[32m+[m[32m            success     = success,[m
[32m+[m[32m            duration_ms = duration_ms,[m
[32m+[m[32m            agent_id    = agent_id,[m
[32m+[m[32m            error       = error,[m
[32m+[m[32m        )[m
[32m+[m[32m        with self._lock:[m
[32m+[m[32m            self._records.append(rec)[m
[32m+[m[32m            # Rolling window — drop oldest[m
[32m+[m[32m            if len(self._records) > self.MAX_RECORDS:[m
[32m+[m[32m                self._records = self._records[-self.MAX_RECORDS:][m
[32m+[m[32m        self._persist()[m
[32m+[m
[32m+[m[32m    def track(self, operation: str, agent_id: str = "system"):[m
[32m+[m[32m        """Context manager for tracking operations."""[m
[32m+[m[32m        return _TrackContext(self, operation, agent_id)[m
[32m+[m
[32m+[m[32m    def snapshot(self, active_agents: int = 0, total_memories: int = 0) -> MetricsSnapshot:[m
[32m+[m[32m        """Generate current reliability snapshot."""[m
[32m+[m[32m        now = time.time()[m
[32m+[m[32m        one_hour_ago  = now - 3600[m
[32m+[m[32m        one_day_ago   = now - 86400[m
[32m+[m[32m        thirty_days_ago = now - 86400 * 30[m
[32m+[m
[32m+[m[32m        with self._lock:[m
[32m+[m[32m            records_1h  = [r for r in self._records if r.timestamp >= one_hour_ago][m
[32m+[m[32m            records_24h = [r for r in self._records if r.timestamp >= one_day_ago][m
[32m+[m[32m            records_30d = [r for r in self._records if r.timestamp >= thirty_days_ago][m
[32m+[m[32m            all_records = self._records[:][m
[32m+[m
[32m+[m[32m        def success_rate(recs):[m
[32m+[m[32m            if not recs:[m
[32m+[m[32m                return 1.0  # No data = assume operational[m
[32m+[m[32m            return sum(1 for r in recs if r.success) / len(recs)[m
[32m+[m
[32m+[m[32m        def percentile(values: list[float], p: float) -> float:[m
[32m+[m[32m            if not values:[m
[32m+[m[32m                return 0.0[m
[32m+[m[32m            sorted_vals = sorted(values)[m
[32m+[m[32m            idx = int(len(sorted_vals) * p / 100)[m
[32m+[m[32m            return sorted_vals[min(idx, len(sorted_vals) - 1)][m
[32m+[m
[32m+[m[32m        latencies_1h = [r.duration_ms for r in records_1h if r.success][m
[32m+[m
[32m+[m[32m        sr_1h  = success_rate(records_1h)[m
[32m+[m[32m        sr_24h = success_rate(records_24h)[m
[32m+[m[32m        sr_30d = success_rate(records_30d)[m
[32m+[m
[32m+[m[32m        # Confidence = weighted average of success rates + latency factor[m
[32m+[m[32m        lat_factor = 1.0[m
[32m+[m[32m        if latencies_1h:[m
[32m+[m[32m            p99 = percentile(latencies_1h, 99)[m
[32m+[m[32m            lat_factor = max(0.5, 1.0 - (p99 / 5000))  # Penalize if p99 > 5s[m
[32m+[m
[32m+[m[32m        confidence = (sr_1h * 0.5 + sr_24h * 0.3 + sr_30d * 0.2) * lat_factor[m
[32m+[m
[32m+[m[32m        return MetricsSnapshot([m
[32m+[m[32m            timestamp           = datetime.now().isoformat(),[m
[32m+[m[32m            liveness            = True,  # If this code runs, service is live[m
[32m+[m[32m            uptime_pct_24h      = round(sr_24h, 4),[m
[32m+[m[32m            uptime_pct_30d      = round(sr_30d, 4),[m
[32m+[m[32m            success_rate_1h     = round(sr_1h, 4),[m
[32m+[m[32m            success_rate_24h    = round(sr_24h, 4),[m
[32m+[m[32m            latency_p50_ms      = round(percentile(latencies_1h, 50), 1),[m
[32m+[m[32m            latency_p95_ms      = round(percentile(latencies_1h, 95), 1),[m
[32m+[m[32m            latency_p99_ms      = round(percentile(latencies_1h, 99), 1),[m
[32m+[m[32m            total_operations    = len(all_records),[m
[32m+[m[32m            operations_last_1h  = len(records_1h),[m
[32m+[m[32m            operations_last_24h = len(records_24h),[m
[32m+[m[32m            confidence_score    = round(confidence, 4),[m
[32m+[m[32m            active_agents       = active_agents,[m
[32m+[m[32m            total_memories_stored = total_memories,[m
[32m+[m[32m        )[m
[32m+[m
[32m+[m[32m    def to_dict(self, **kwargs) -> dict:[m
[32m+[m[32m        return asdict(self.snapshot(**kwargs))[m
[32m+[m
[32m+[m[32m    def _persist(self):[m
[32m+[m[32m        """Save recent records to disk for persistence across restarts."""[m
[32m+[m[32m        try:[m
[32m+[m[32m            recent = self._records[-1000:]  # Save last 1000[m
[32m+[m[32m            data = [asdict(r) for r in recent][m
[32m+[m[32m            with open(self._persist_path, "w") as f:[m
[32m+[m[32m                json.dump(data, f)[m
[32m+[m[32m        except Exception:[m
[32m+[m[32m            pass[m
[32m+[m
[32m+[m[32m    def _load(self):[m
[32m+[m[32m        """Load persisted records on startup."""[m
[32m+[m[32m        try:[m
[32m+[m[32m            if self._persist_path.exists():[m
[32m+[m[32m                with open(self._persist_path) as f:[m
[32m+[m[32m                    data = json.load(f)[m
[32m+[m[32m                self._records = [OperationRecord(**r) for r in data][m
[32m+[m[32m        except Exception:[m
[32m+[m[32m            self._records = [][m
[32m+[m
[32m+[m
[32m+[m[32mclass _TrackContext:[m
[32m+[m[32m    """Context manager returned by MetricsCollector.track()."""[m
[32m+[m[32m    def __init__(self, collector: MetricsCollector, operation: str, agent_id: str):[m
[32m+[m[32m        self._collector = collector[m
[32m+[m[32m        self._operation = operation[m
[32m+[m[32m        self._agent_id  = agent_id[m
[32m+[m[32m        self._start     = None[m
[32m+[m
[32m+[m[32m    def __enter__(self):[m
[32m+[m[32m        self._start = time.time()[m
[32m+[m[32m        return self[m
[32m+[m
[32m+[m[32m    def __exit__(self, exc_type, exc_val, exc_tb):[m
[32m+[m[32m        duration_ms = (time.time() - self._start) * 1000[m
[32m+[m[32m        success = exc_type is None[m
[32m+[m[32m        error = str(exc_val) if exc_val else None[m
[32m+[m[32m        self._collector.record(self._operation, self._agent_id, success, duration_ms, error)[m
[32m+[m[32m        return False  # Don't suppress exceptions[m
[32m+[m
[32m+[m
[32m+[m[32m# Global singleton — shared across server.py and mcp_server.py[m
[32m+[m[32m_global_metrics = MetricsCollector()[m
[32m+[m
[32m+[m[32mdef get_metrics() -> MetricsCollector:[m
[32m+[m[32m    return _global_metrics[m
[1mdiff --git a/server.py b/server.py[m
[1mnew file mode 100644[m
[1mindex 0000000..0ea3661[m
[1m--- /dev/null[m
[1m+++ b/server.py[m
[36m@@ -0,0 +1,216 @@[m
[32m+[m[32m"""[m
[32m+[m[32mcore/server.py  (v3.1 — Agent Discovery Layer)[m
[32m+[m
[32m+[m[32mFastAPI server with three layers Gary Tan describes:[m
[32m+[m[32m  1. API pública         → all existing endpoints[m
[32m+[m[32m  2. Manifest discovery  → GET /.well-known/agent.json[m
[32m+[m[32m  3. Métricas en tiempo real → GET /metrics  (liveness + reliability + confidence)[m
[32m+[m[32m"""[m
[32m+[m
[32m+[m[32mimport os[m
[32m+[m[32mimport json[m
[32m+[m[32mimport time[m
[32m+[m[32mimport uuid[m
[32m+[m[32mfrom datetime import datetime[m
[32m+[m[32mfrom typing import Optional[m
[32m+[m[32mfrom pathlib import Path[m
[32m+[m
[32m+[m[32mfrom fastapi import FastAPI, HTTPException[m
[32m+[m[32mfrom fastapi.middleware.cors import CORSMiddleware[m
[32m+[m[32mfrom fastapi.responses import JSONResponse[m
[32m+[m[32mfrom pydantic import BaseModel, Field[m
[32m+[m
[32m+[m[32mfrom core.agent import Agent, MockAdapter, VertexAIAdapter, AnthropicAdapter[m
[32m+[m[32mfrom core.memory_engine import MemoryType, Importance, LocalStorage[m
[32m+[m[32mfrom core.metrics import get_metrics[m
[32m+[m
[32m+[m[32mLLM_BACKEND  = os.environ.get("LLM_BACKEND", "mock")[m
[32m+[m[32mLLM_MODEL    = os.environ.get("LLM_MODEL", None)[m
[32m+[m[32mIDENTITY_DIR = os.environ.get("IDENTITY_DIR", "./identity")[m
[32m+[m[32mDATA_DIR     = os.environ.get("DATA_DIR", "./data/agents")[m
[32m+[m
[32m+[m[32mmetrics = get_metrics()[m
[32m+[m[32m_agents: dict[str, Agent] = {}[m
[32m+[m
[32m+[m[32mdef build_llm():[m
[32m+[m[32m    if LLM_BACKEND == "vertex":[m
[32m+[m[32m        return VertexAIAdapter(model=LLM_MODEL or "gemini-2.0-flash-exp")[m
[32m+[m[32m    elif LLM_BACKEND == "anthropic":[m
[32m+[m[32m        return AnthropicAdapter(model=LLM_MODEL or "claude-sonnet-4-20250514")[m
[32m+[m[32m    return MockAdapter()[m
[32m+[m
[32m+[m[32mdef get_agent(agent_id: str, agent_type: str = "general") -> Agent:[m
[32m+[m[32m    if agent_id not in _agents:[m
[32m+[m[32m        _agents[agent_id] = Agent([m
[32m+[m[32m            agent_id=agent_id, agent_type=agent_type,[m
[32m+[m[32m            llm=build_llm(), identity_dir=IDENTITY_DIR, data_dir=DATA_DIR,[m
[32m+[m[32m        )[m
[32m+[m[32m    return _agents[agent_id][m
[32m+[m
[32m+[m[32mdef total_memories() -> int:[m
[32m+[m[32m    try:[m
[32m+[m[32m        return sum(get_agent(aid).stats()["total_memories"] for aid in LocalStorage(DATA_DIR).list_agents())[m
[32m+[m[32m    except:[m
[32m+[m[32m        return 0[m
[32m+[m
[32m+[m[32mclass InteractRequest(BaseModel):[m
[32m+[m[32m    session_id: str = Field(default_factory=lambda: f"sess_{uuid.uuid4().hex[:8]}")[m
[32m+[m[32m    agent_id: str = "default"[m
[32m+[m[32m    agent_type: str = "general"[m
[32m+[m[32m    user_input: str[m
[32m+[m[32m    explicit_importance: Optional[str] = None[m
[32m+[m
[32m+[m[32mclass MemoryQueryRequest(BaseModel):[m
[32m+[m[32m    agent_id: str[m
[32m+[m[32m    query: str[m
[32m+[m[32m    memory_type: Optional[str] = None[m
[32m+[m[32m    limit: int = 10[m
[32m+[m
[32m+[m[32mclass RememberRequest(BaseModel):[m
[32m+[m[32m    agent_id: str[m
[32m+[m[32m    fact: str[m
[32m+[m[32m    memory_type: Optional[str] = "long_term"[m
[32m+[m
[32m+[m[32mclass ConsolidateRequest(BaseModel):[m
[32m+[m[32m    agent_id: str[m
[32m+[m
[32m+[m[32mapp = FastAPI([m
[32m+[m[32m    title="Agent Memory", description="Autonomous cognitive memory layer. MCP-compatible.",[m
[32m+[m[32m    version="3.1.0", docs_url="/docs",[m
[32m+[m[32m)[m
[32m+[m[32mapp.add_middleware(CORSMiddleware, allow_origins=["*"], allow_methods=["*"], allow_headers=["*"])[m
[32m+[m
[32m+[m[32m@app.get("/.well-known/agent.json", tags=["Discovery"])[m
[32m+[m[32mdef agent_manifest():[m
[32m+[m[32m    """Machine-readable capability manifest. Discovered by agents before they decide to use this service."""[m
[32m+[m[32m    manifest_path = Path(__file__).parent.parent / ".well-known" / "agent.json"[m
[32m+[m[32m    if manifest_path.exists():[m
[32m+[m[32m        return JSONResponse(content=json.loads(manifest_path.read_text()))[m
[32m+[m[32m    raise HTTPException(status_code=404, detail="Manifest not found")[m
[32m+[m
[32m+[m[32m@app.get("/metrics", tags=["Discovery"])[m
[32m+[m[32mdef reliability_metrics():[m
[32m+[m[32m    """Real-time reliability metrics: liveness, reliability, confidence. Used by agent scoring functions."""[m
[32m+[m[32m    snap = metrics.snapshot(active_agents=len(_agents), total_memories=total_memories())[m
[32m+[m[32m    return {[m
[32m+[m[32m        "liveness": snap.liveness,[m
[32m+[m[32m        "reliability": {[m
[32m+[m[32m            "success_rate_1h": snap.success_rate_1h,[m
[32m+[m[32m            "success_rate_24h": snap.success_rate_24h,[m
[32m+[m[32m            "uptime_pct_24h": snap.uptime_pct_24h,[m
[32m+[m[32m            "uptime_pct_30d": snap.uptime_pct_30d,[m
[32m+[m[32m        },[m
[32m+[m[32m        "confidence": snap.confidence_score,[m
[32m+[m[32m        "latency": {"p50_ms": snap.latency_p50_ms, "p95_ms": snap.latency_p95_ms, "p99_ms": snap.latency_p99_ms},[m
[32m+[m[32m        "usage": {[m
[32m+[m[32m            "total_operations": snap.total_operations,[m
[32m+[m[32m            "operations_last_1h": snap.operations_last_1h,[m
[32m+[m[32m            "operations_last_24h": snap.operations_last_24h,[m
[32m+[m[32m            "active_agents": snap.active_agents,[m
[32m+[m[32m            "total_memories": snap.total_memories_stored,[m
[32m+[m[32m        },[m
[32m+[m[32m        "timestamp": snap.timestamp,[m
[32m+[m[32m    }[m
[32m+[m
[32m+[m[32m@app.get("/health", tags=["Discovery"])[m
[32m+[m[32mdef health():[m
[32m+[m[32m    return {"status": "ok", "liveness": True, "version": "3.1.0",[m
[32m+[m[32m            "llm_backend": LLM_BACKEND, "timestamp": datetime.now().isoformat(),[m
[32m+[m[32m            "manifest": "GET /.well-known/agent.json", "metrics": "GET /metrics",[m
[32m+[m[32m            "mcp": "python mcp_server.py"}[m
[32m+[m
[32m+[m[32m@app.get("/", tags=["Info"])[m
[32m+[m[32mdef root():[m
[32m+[m[32m    return {"name": "Agent Memory", "version": "3.1.0",[m
[32m+[m[32m            "discovery": {"manifest": "GET /.well-known/agent.json", "metrics": "GET /metrics", "mcp": "python mcp_server.py"},[m
[32m+[m[32m            "api": {"interact": "POST /interact", "remember": "POST /memory/remember",[m
[32m+[m[32m                    "query": "POST /memory/query", "context": "GET /memory/context/{agent_id}",[m
[32m+[m[32m                    "stats": "GET /memory/stats/{agent_id}", "docs": "GET /docs"}}[m
[32m+[m
[32m+[m[32m@app.post("/interact", tags=["Agent"])[m
[32m+[m[32masync def interact(req: InteractRequest):[m
[32m+[m[32m    t0 = time.time()[m
[32m+[m[32m    try:[m
[32m+[m[32m        agent = get_agent(req.agent_id, req.agent_type)[m
[32m+[m[32m        explicit = Importance(req.explicit_importance) if req.explicit_importance else None[m
[32m+[m[32m        with metrics.track("interact", agent_id=req.agent_id):[m
[32m+[m[32m            response = agent.interact(session_id=req.session_id, user_input=req.user_input, explicit_importance=explicit)[m
[32m+[m[32m        return {"agent_id": response.agent_id, "session_id": response.session_id,[m
[32m+[m[32m                "response": response.text,[m
[32m+[m[32m                "thought": {"importance": response.thought.importance_score, "memories_stored": len(response.thought.memories_stored), "duration_ms": response.thought.duration_ms},[m
[32m+[m[32m                "memory_stats": response.memory_stats, "timestamp": datetime.now().isoformat()}[m
[32m+[m[32m    except Exception as e:[m
[32m+[m[32m        metrics.record("interact", req.agent_id, False, (time.time()-t0)*1000, str(e))[m
[32m+[m[32m        raise HTTPException(status_code=500, detail=str(e))[m
[32m+[m
[32m+[m[32m@app.post("/memory/remember", tags=["Memory"])[m
[32m+[m[32masync def remember(req: RememberRequest):[m
[32m+[m[32m    try:[m
[32m+[m[32m        agent = get_agent(req.agent_id)[m
[32m+[m[32m        mt = MemoryType(req.memory_type) if req.memory_type else MemoryType.LONG_TERM[m
[32m+[m[32m        with metrics.track("store", agent_id=req.agent_id):[m
[32m+[m[32m            mem = agent.memory.store(mt, req.fact, importance=Importance.CRITICAL, tags=["explicit"])[m
[32m+[m[32m        return {"status": "stored", "memory_id": mem.id, "type": mem.type.value}[m
[32m+[m[32m    except Exception as e:[m
[32m+[m[32m        raise HTTPException(status_code=500, detail=str(e))[m
[32m+[m
[32m+[m[32m@app.post("/memory/query", tags=["Memory"])[m
[32m+[m[32masync def query_memory(req: MemoryQueryRequest):[m
[32m+[m[32m    try:[m
[32m+[m[32m        agent = get_agent(req.agent_id)[m
[32m+[m[32m        mt = MemoryType(req.memory_type) if req.memory_type else None[m
[32m+[m[32m        with metrics.track("retrieve", agent_id=req.agent_id):[m
[32m+[m[32m            memories = agent.memory.retrieve(memory_type=mt, limit=req.limit, query=req.query)[m
[32m+[m[32m        return {"agent_id": req.agent_id, "count": len(memories), "memories": [m.to_dict() for m in memories]}[m
[32m+[m[32m    except Exception as e:[m
[32m+[m[32m        raise HTTPException(status_code=500, detail=str(e))[m
[32m+[m
[32m+[m[32m@app.post("/memory/consolidate", tags=["Memory"])[m
[32m+[m[32masync def consolidate(req: ConsolidateRequest):[m
[32m+[m[32m    try:[m
[32m+[m[32m        agent = get_agent(req.agent_id)[m
[32m+[m[32m        with metrics.track("consolidate", agent_id=req.agent_id):[m
[32m+[m[32m            new_mems = agent.memory.consolidate()[m
[32m+[m[32m        return {"agent_id": req.agent_id, "consolidated": len(new_mems)}[m
[32m+[m[32m    except Exception as e:[m
[32m+[m[32m        raise HTTPException(status_code=500, detail=str(e))[m
[32m+[m
[32m+[m[32m@app.get("/memory/stats/{agent_id}", tags=["Memory"])[m
[32m+[m[32masync def memory_stats(agent_id: str):[m
[32m+[m[32m    return get_agent(agent_id).stats()[m
[32m+[m
[32m+[m[32m@app.get("/memory/context/{agent_id}", tags=["Memory"])[m
[32m+[m[32masync def memory_context(agent_id: str, limit_per_layer: int = 3):[m
[32m+[m[32m    try:[m
[32m+[m[32m        agent = get_agent(agent_id)[m
[32m+[m[32m        ctx = agent.memory.retrieve_context(limit_per_layer=limit_per_layer)[m
[32m+[m[32m        return {"agent_id": agent_id, "context_text": agent.memory.format_context_for_prompt(limit_per_layer),[m
[32m+[m[32m                "layers": {layer: [m.to_dict() for m in mems] for layer, mems in ctx.items()}}[m
[32m+[m[32m    except Exception as e:[m
[32m+[m[32m        raise HTTPException(status_code=500, detail=str(e))[m
[32m+[m
[32m+[m[32m@app.delete("/memory/session/{agent_id}", tags=["Memory"])[m
[32m+[m[32masync def clear_session(agent_id: str):[m
[32m+[m[32m    get_agent(agent_id).new_session()[m
[32m+[m[32m    return {"status": "cleared", "long_term_preserved": True}[m
[32m+[m
[32m+[m[32m@app.delete("/memory/agent/{agent_id}", tags=["Memory"])[m
[32m+[m[32masync def clear_agent(agent_id: str):[m
[32m+[m[32m    agent = get_agent(agent_id)[m
[32m+[m[32m    agent.memory.storage.delete(agent_id)[m
[32m+[m[32m    if agent_id in _agents:[m
[32m+[m[32m        del _agents[agent_id][m
[32m+[m[32m    return {"status": "deleted", "agent_id": agent_id}[m
[32m+[m
[32m+[m[32m@app.get("/agents", tags=["Info"])[m
[32m+[m[32mdef list_agents():[m
[32m+[m[32m    storage = LocalStorage(DATA_DIR)[m
[32m+[m[32m    result = [][m
[32m+[m[32m    for aid in storage.list_agents():[m
[32m+[m[32m        s = get_agent(aid).stats()[m
[32m+[m[32m        result.append({"agent_id": aid, "total_memories": s["total_memories"], "memory_pressure": s["memory_pressure"]})[m
[32m+[m[32m    return {"agents": result, "count": len(result)}[m
[32m+[m
[32m+[m[32mif __name__ == "__main__":[m
[32m+[m[32m    import uvicorn[m
[32m+[m[32m    uvicorn.run(app, host="0.0.0.0", port=int(os.environ.get("PORT", 8080)))[m
[1mdiff --git a/test.py b/test.py[m
[1mnew file mode 100644[m
[1mindex 0000000..e69de29[m
