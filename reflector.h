/*
 * Copyright (C) 2017 Rockchip Electronics Co., Ltd.
 * author: hertz.wang hertz.wong@rock-chips.com
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef RKMEDIA_REFLECTOR_H_
#define RKMEDIA_REFLECTOR_H_

#include <map>
#include <memory>
#include <string>

// macro of declare reflector
#define DECLARE_REFLECTOR(PRODUCT, FACTORY, REFLECTOR)                         \
  class PRODUCT;                                                               \
  class FACTORY;                                                               \
  class REFLECTOR {                                                            \
  public:                                                                      \
    static REFLECTOR &Instance();                                              \
    std::shared_ptr<PRODUCT> Create(const char *request, const char *name);    \
    void RegisterFactory(std::string identifier, FACTORY *factory);            \
                                                                               \
  private:                                                                     \
    REFLECTOR() = default;                                                     \
    ~REFLECTOR() = default;                                                    \
    REFLECTOR(const REFLECTOR &) = delete;                                     \
    REFLECTOR &operator=(const REFLECTOR &) = delete;                          \
                                                                               \
    std::map<std::string, FACTORY *> factorys_;                                \
  };

// macro of define reflector
#define DEFINE_REFLECTOR(PRODUCT, FACTORY, REFLECTOR)                          \
  REFLECTOR &REFLECTOR::Instance() {                                           \
    static REFLECTOR _reflector;                                               \
    return _reflector;                                                         \
  }                                                                            \
                                                                               \
  void REFLECTOR::RegisterFactory(std::string identifier, FACTORY *factory) {  \
    auto it = factorys_.find(identifier);                                      \
    if (it == factorys_.end())                                                 \
      factorys_[identifier] = factory;                                         \
    else                                                                       \
      fprintf(stderr, "repeated identifier : %s\n", identifier.c_str());       \
  }                                                                            \
                                                                               \
  std::shared_ptr<PRODUCT> REFLECTOR::Create(const char *request,              \
                                             const char *name) {               \
    const char *identifier = FACTORY::Parse(request);                          \
    if (!identifier)                                                           \
      return nullptr;                                                          \
                                                                               \
    auto it = factorys_.find(identifier);                                      \
    if (it != factorys_.end()) {                                               \
      FACTORY *f = it->second;                                                 \
      return f->NewProduct(name);                                              \
    }                                                                          \
    return nullptr;                                                            \
  }

// macro of declare base factory
#define DECLARE_FACTORY(PRODUCT, FACTORY)                                      \
  class PRODUCT;                                                               \
  class FACTORY {                                                              \
  public:                                                                      \
    virtual const char *Identifier() = 0;                                      \
    static const char *Parse(const char *request);                             \
    virtual std::shared_ptr<PRODUCT> NewProduct(const char *name) = 0;         \
                                                                               \
  protected:                                                                   \
    FACTORY() = default;                                                       \
    virtual ~FACTORY() = default;                                              \
                                                                               \
  private:                                                                     \
    FACTORY(const FACTORY &) = delete;                                         \
    FACTORY &operator=(const FACTORY &) = delete;                              \
  };

#define FACTORY_IDENTIFIER_DEFINITION(IDENTIFIER)                              \
  const char *Identifier() override { return IDENTIFIER; }

#define FACTORY_INSTANCE_DEFINITION(FACTORY)                                   \
  static FACTORY &Instance() {                                                 \
    static FACTORY object;                                                     \
    return object;                                                             \
  }

#define FACTORY_REGISTER(FACTORY, REFLECTOR)                                   \
  class Register_##FACTORY {                                                   \
  public:                                                                      \
    Register_##FACTORY() {                                                     \
      FACTORY &obj = FACTORY::Instance();                                      \
      REFLECTOR::Instance().RegisterFactory(obj.Identifier(), &obj);           \
    }                                                                          \
  };                                                                           \
  Register_##FACTORY reg_##FACTORY;

// macro of define child factory
#define DEFINE_CHILD_FACTORY(CHILD_PRODUCT, CHILD_FACTORY, IDENTIFIER,         \
                             PRODUCT, FACTORY, REFLECTOR)                      \
  class CHILD_FACTORY : public FACTORY {                                       \
  public:                                                                      \
    FACTORY_IDENTIFIER_DEFINITION(IDENTIFIER)                                  \
    std::shared_ptr<PRODUCT> NewProduct(const char *name) override {           \
      return std::shared_ptr<CHILD_PRODUCT>(new CHILD_PRODUCT(name));          \
    }                                                                          \
    FACTORY_INSTANCE_DEFINITION(CHILD_FACTORY)                                 \
                                                                               \
  private:                                                                     \
    CHILD_FACTORY() = default;                                                 \
    ~CHILD_FACTORY() = default;                                                \
  };                                                                           \
                                                                               \
  FACTORY_REGISTER(CHILD_FACTORY, REFLECTOR)

#endif // RKMEDIA_REFLECTOR_H_
