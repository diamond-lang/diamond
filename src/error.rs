#[derive(Debug, Clone)]
pub struct Error {
    pub message: String,
    pub line: usize,
    pub column: usize,
}
