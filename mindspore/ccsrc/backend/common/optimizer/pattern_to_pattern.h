/**
 * Copyright 2022 Huawei Technologies Co., Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef MINDSPORE_PATTERN_TO_PATTERN_H
#define MINDSPORE_PATTERN_TO_PATTERN_H

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "utils/hash_map.h"
#include "ir/anf.h"
#include "ir/func_graph.h"
#include "ir/primitive.h"
#include "backend/common/optimizer/pass_manager.h"
#include "backend/common/optimizer/pattern_engine.h"
#include "ir/graph_utils.h"
#include "utils/ms_utils.h"
#include "backend/common/optimizer/helper.h"
#include "backend/common/optimizer/optimizer.h"
#include "include/backend/visible.h"
#include "base/base.h"
#include "base/base_ref.h"

namespace mindspore {
namespace opt {
bool BACKEND_EXPORT AlwaysReturnTrue(const BaseRef &n);

class BACKEND_EXPORT PatternMap {
 public:
  PatternMap() = default;
  bool Contains(const std::string &name) const;
  bool CheckSeq(const std::string &name) const;
  AnfNodePtr Get(const std::string &name) const;
  const std::vector<AnfNodePtr> &GetSeq(const std::string &name) const;
  bool Emplace(const std::string &name, const AnfNodePtr &node);
  bool Emplace(const std::string &name, const std::vector<AnfNodePtr> &v);
  void Clear();
  bool Check(const std::string &name, const AnfNodePtr &node) const;
  void Erase(const mindspore::HashSet<std::string> &del_set);

 private:
  mindspore::HashSet<std::string> name_set_;
  mindspore::HashMap<std::string, AnfNodePtr> node_map_;
  mindspore::HashMap<std::string, std::vector<AnfNodePtr>> seq_map_;
};

using PatternMapPtr = std::shared_ptr<PatternMap>;
using BuildCNodeFunc = std::function<AnfNodePtr(const PatternMap &, const AnfNodePtr &)>;
using BuildValueFunc = std::function<AnfNodePtr(const PatternMap &)>;

class BACKEND_EXPORT DefaultCNodeFunc {
 public:
  DefaultCNodeFunc() = default;
  AnfNodePtr operator()(const PatternMap &, const AnfNodePtr &default_cnode) const { return default_cnode; }
};

class BACKEND_EXPORT InplaceCNodeFunc {
 public:
  explicit InplaceCNodeFunc(std::string s) : s_(std::move(s)) {}
  AnfNodePtr operator()(const PatternMap &m, const AnfNodePtr & /* default_cnode */) const { return m.Get(s_); }

 private:
  std::string s_;
};

class BACKEND_EXPORT DefaultValueFunc {
 public:
  explicit DefaultValueFunc(ValuePtr v) : v_(std::move(v)) {}
  AnfNodePtr operator()(const PatternMap &) const { return NewValueNode(v_); }

 private:
  ValuePtr v_;
};

class BACKEND_EXPORT InplaceValueFunc {
 public:
  explicit InplaceValueFunc(std::string s) : s_(std::move(s)) {}
  AnfNodePtr operator()(const PatternMap &m) const { return m.Get(s_); }

 private:
  std::string s_;
};

class BACKEND_EXPORT PatternToPatternPass;
class BACKEND_EXPORT UnpackNode {
 public:
  UnpackNode &operator=(const std::string &name);
  UnpackNode &operator=(const UnpackNode &u) = default;

 private:
  explicit UnpackNode(AnfNodePtr node) : node_(std::move(node)) {}
  AnfNodePtr node_ = nullptr;
  std::string key_;
  friend class DstPattern;
  friend class PatternToPatternPass;
};

class BACKEND_EXPORT PatternNode {
 public:
  PatternNode(const PrimitivePtr &p) : type_("prim"), p_(NewValueNode(p)) {}  // NOLINT
  PatternNode(const char *name) : type_("name"), name_(name) {}               // NOLINT
  PatternNode(std::vector<UnpackNode> &v) : type_("unpack"), v_(v) {}         // NOLINT
  PatternNode(const PatternNode &) = default;

 private:
  std::string type_;
  std::string name_;
  ValueNodePtr p_;
  std::vector<UnpackNode> v_;
  friend class SrcPattern;
  friend class DstPattern;
};

class BACKEND_EXPORT SrcPattern {
 public:
  SrcPattern &AddVar(const std::string &name, const ConditionFunc &f = AlwaysReturnTrue);
  SrcPattern &AddSeqVar(const std::string &name, const ConditionFunc &f = AlwaysReturnTrue);
  const BaseRef &GetRef(const std::string &name) const;
  SrcPattern &AddCNode(const std::string &name, const std::initializer_list<PatternNode> &v);
  BaseRef GetRoot() const;

 private:
  explicit SrcPattern(PatternMapPtr m) : m_(std::move(m)), has_root_(false) {}
  bool match(const std::string &name, const AnfNodePtr &node, const EquivPtr &equiv);
  bool build_pattern_map(const AnfNodePtr &node, const EquivPtr &equiv);
  PatternMapPtr m_;
  mindspore::HashMap<std::string, BaseRef> ref_map_;
  mindspore::HashMap<std::string, std::vector<PatternNode>> inputs_map_;
  bool has_root_;
  std::string root_;
  friend class PatternToPatternPass;
};

class BACKEND_EXPORT DstPattern {
 public:
  DstPattern &AddCNode(const string &name, const std::initializer_list<PatternNode> &inputs,
                       const BuildCNodeFunc &buildfunc = DefaultCNodeFunc());
  DstPattern &AddValueNode(const string &name, const BuildValueFunc &buildfunc);

 private:
  explicit DstPattern(PatternMapPtr m) : m_(std::move(m)) {}
  AnfNodePtr Root();
  void clear();
  void set_info(PatternToPatternPass *now_pass, const FuncGraphPtr &func_graph);
  friend class PatternToPatternPass;
  PatternMapPtr m_;
  mindspore::HashSet<std::string> dst_set_;
  bool fail_ = false;
  AnfNodePtr root_ = nullptr;
  FuncGraphPtr fg_ = nullptr;
  PatternToPatternPass *pass_ = nullptr;
};

class BACKEND_EXPORT PatternToPatternPass : public PatternPass {
 public:
  explicit PatternToPatternPass(const std::string &name = "", bool multigraph = true)
      : PatternPass(name, multigraph),
        m_(std::make_shared<PatternMap>()),
        src_pattern_(SrcPattern(m_)),
        dst_pattern_(DstPattern(m_)) {}
  ~PatternToPatternPass() override = default;
  virtual void DefineSrcPattern(SrcPattern *src_pattern) = 0;
  virtual void DefineDstPattern(DstPattern *dst_pattern) = 0;
  virtual bool CheckMatchedDAG(const PatternMap &, const FuncGraphPtr &, const AnfNodePtr &) const = 0;
  AnfNodePtr Run(const FuncGraphPtr &func_graph, const AnfNodePtr &node) override;
  std::vector<UnpackNode> Unpacking(const std::string &s);

 private:
  PatternMapPtr m_;
  SrcPattern src_pattern_;
  DstPattern dst_pattern_;
  AnfNodePtr src_pattern_root_ = nullptr;
};
}  // namespace opt
}  // namespace mindspore

#endif  // MINDSPORE_PATTERN_TO_PATTERN_H
