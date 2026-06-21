import React from 'react';
import { Modal } from 'antd';
import { motion } from 'framer-motion';

interface GlassModalProps {
  open: boolean;
  onCancel: () => void;
  title?: string;
  width?: number;
  children: React.ReactNode;
  footer?: React.ReactNode;
}

export default function GlassModal({ open, onCancel, title, width = 700, children, footer }: GlassModalProps) {
  return (
    <Modal
      open={open}
      onCancel={onCancel}
      title={title}
      width={width}
      footer={footer}
      centered
      destroyOnClose
      className="glass-modal"
    >
      <motion.div
        initial={{ scale: 0.95, opacity: 0 }}
        animate={{ scale: 1, opacity: 1 }}
        transition={{ duration: 0.25, ease: 'easeOut' }}
      >
        {children}
      </motion.div>
    </Modal>
  );
}
