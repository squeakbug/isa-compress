#ifndef COMMAND_H
#define COMMAND_H

#include <exception>
#include <stdexcept>
#include <ostream>

#include "dynbitset.h"

namespace utils
{

class command : public dynbitset
{
public:
    void devide(command &ocmd1, command &ocmd2, size_t fbits) const;

    void devide_half(command &ocmd1, command &ocmd2) const;

    friend std::ostream& operator<<(std::ostream& os, const command &cmd);

private:
    void devide_impl(command &ocmd1, command &ocmd2, size_t fbits, const command &icmd) const;
};

bool operator<(const command &c1, const command &c2);

bool operator==(const command &c1, const command &c2);

bool operator!=(const command &c1, const command &c2);

std::ostream &operator<<(std::ostream &os, const utils::command &cmd);

}

#endif
