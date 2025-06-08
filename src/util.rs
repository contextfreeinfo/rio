use std::{collections::HashMap, hash::Hash};

#[derive(Default)]
pub struct MultiMap<K, V>
where
    K: Copy + Hash + Eq + Ord,
    V: MultiMapEntry<K>,
{
    pub map: HashMap<K, u32>,
    // Single vector to limit allocations.
    pub values: Vec<V>,
}

pub trait MultiMapEntry<K> {
    fn key(&self) -> K;
}

impl<K, V> MultiMap<K, V>
where
    K: Copy + Hash + Eq + Ord,
    V: MultiMapEntry<K>,
{
    pub fn new() -> Self {
        Self {
            map: HashMap::new(),
            values: vec![],
        }
    }

    pub fn get(&self, key: K) -> MultiMapIterator<'_, K, V> {
        match self.map.get(&key) {
            Some(index) => MultiMapIterator {
                index: *index as usize,
                key,
                values: &self.values,
            },
            None => MultiMapIterator {
                index: 0,
                key,
                values: &[],
            },
        }
    }

    pub fn update(&mut self, sort_map: bool) {
        if sort_map {
            self.values.sort_by_key(|value| value.key())
        }
        self.map.clear();
        let Some(first) = self.values.first() else {
            return;
        };
        let mut last_key = first.key();
        self.map.insert(last_key, 0);
        for (index, value) in self.values.iter().enumerate().skip(1) {
            let key = value.key();
            if key != last_key {
                self.map.insert(key, index as u32);
                last_key = key;
            }
        }
    }
}

pub struct MultiMapIterator<'a, K, V> {
    index: usize,
    key: K,
    values: &'a [V],
}

impl<'a, K, V> Iterator for MultiMapIterator<'a, K, V>
where
    K: Copy + Hash + Eq + Ord,
    V: MultiMapEntry<K>,
{
    type Item = &'a V;

    fn next(&mut self) -> Option<Self::Item> {
        let value = self.values.get(self.index)?;
        if value.key() == self.key {
            self.index += 1;
            Some(value)
        } else {
            None
        }
    }
}
