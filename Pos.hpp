struct Pos
{
    int x, y;

    Pos(int x, int y);
    Pos();

    bool operator==(const Pos& other) const;
    bool operator !=(const Pos& other) const;
    void operator+=(const Pos& other);
    void operator-=(const Pos& other);
    Pos operator+(const Pos& other) const;
    Pos operator-(const Pos& other) const;
    Pos operator*(int a) const;
    bool operator<(const Pos& other) const;
};

