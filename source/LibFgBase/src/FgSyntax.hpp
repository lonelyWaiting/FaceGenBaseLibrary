//
// Copyright (c) 2015 Singular Inversions Inc. (facegen.com)
// Use, modification and distribution is subject to the MIT License,
// see accompanying file LICENSE.txt or facegen.com/base_library_license.txt
//
// Authors: Andrew Beatty
// Date: Nov 27, 2010
//
// Make command-line syntax handling easier

#ifndef FGSYNTAX_HPP
#define FGSYNTAX_HPP

#include "FgStdLibs.hpp"
#include "FgStdString.hpp"
#include "FgMain.hpp"

struct  FgExceptionCommandSyntax
{};

struct  FgSyntax
{
    FgSyntax(const FgArgs & args,const std::string & syntax);

    ~FgSyntax();

    const std::string &
    next();

    std::string
    nextLower()             // As above but lower case
    {return fgToLower(next()); }

    const std::string &
    curr() const
    {return m_args[m_idx]; }

    bool
    more() const
    {return (m_idx+1 < m_args.size()); }

    const std::string &
    peekNext();

    FgArgs
    rest();             // Starting with current

    void
    error()
    {throwSyntax(); }

    void
    error(const std::string & errMsg);

    void
    error(const std::string & errMsg,const std::string & data);

    void
    incorrectNumArgs();

    // Throws appropriate syntax error:
    void
    checkExtension(
        const std::string & fname,
        const std::string & ext);
    void
    checkExtension(
        const std::string &                 fname,
        const std::vector<std::string> &    exts);

    // Throws appropriate syntax error if different:
    void
    numArgsMustBe(uint numArgsNotIncludingCommand);

private:
    std::string                 m_syntax;
    std::vector<std::string>    m_args;
    size_t                      m_idx;

    void
    throwSyntax();
};

#endif

// */