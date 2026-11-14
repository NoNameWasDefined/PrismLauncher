// File overview: Base for themes based on Qt's Fusion style.

#pragma once

#include "ITheme.h"

class FusionTheme : public ITheme {
   public:
    virtual ~FusionTheme() {}

    QString qtTheme() override;
};
